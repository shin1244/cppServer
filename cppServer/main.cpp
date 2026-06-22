#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <thread>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <stack>
#include <atomic>
#include"RingBuffer.h"

#pragma pack(push, 1)
struct PacketHeader {
    unsigned short size; 
    unsigned short id;  
};
#pragma pack(pop)

const int HEADER_SIZE = 4;

struct Session {
    SOCKET socket;
    int index;

    RingBuffer recvBuffer;
    RingBuffer sendBuffer;

    bool sendPending;

    OVERLAPPED recvOverlapped;
    OVERLAPPED sendOverlapped;
    WSABUF recvWsaBuf;
    WSABUF sendWsaBuf;
};

HANDLE g_iocp;   // IOCP 핸들
Session g_sessionList[1000];
std::stack<int> g_freeIndices;

void postRecv(Session*);
void postSend(Session*, const char*, int);
void flushSend(Session*);
void initPool();
int allocSession();
void freeSession(int index);



// 워커 스레드: 완료 큐를 계속 기다림
void workerThread() {
    while (true) {
        DWORD bytesTransferred = 0;
        ULONG_PTR completionKey = 0;
        OVERLAPPED* overlapped = nullptr;

        BOOL ok = GetQueuedCompletionStatus(
            g_iocp,
            &bytesTransferred, 
            &completionKey,   
            &overlapped,     
            INFINITE     
        );

        Session* session = (Session*)completionKey;

        if (!ok) {
            freeSession(session->index);
            closesocket(session->socket);
            std::cout << "[session " << session->socket << "] Disconnect\n";
            continue;
        }

        if (overlapped == &session->recvOverlapped) {
            session->recvBuffer.OnWrite(bytesTransferred);

            while (true) {
                if (session->recvBuffer.GetUsedSize() < HEADER_SIZE) break;

                PacketHeader header;
                session->recvBuffer.Peek((char*)&header, HEADER_SIZE);

                std::cout << "header: " << header.size << "bytes\n";

                if (session->recvBuffer.GetUsedSize() < header.size) break;

                char packet[4096];
                session->recvBuffer.Peek(packet, header.size);
                session->recvBuffer.OnRead(header.size);

                postSend(session, packet, header.size);
            }
            postRecv(session);
        }
        else if (overlapped == &session->sendOverlapped){
            std::cout << "[worker] sent " << bytesTransferred << " bytes\n";
            session->sendBuffer.OnRead(bytesTransferred);
            session->sendPending = false;
            flushSend(session);
        }
    }
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    std::cout << "Winsock ready\n";

    initPool();

    g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (g_iocp == NULL) {
        std::cout << "CreateIoCompletionPort failed: " << GetLastError() << "\n";
        return 1;
    }
    std::cout << "IOCP created\n";

    unsigned int n = std::thread::hardware_concurrency();
    std::cout << "spawning " << n << " worker threads\n";

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cout << "socket failed: " << WSAGetLastError() << "\n";
        return 1;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(5050);
    if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cout << "bind failed: " << WSAGetLastError() << "\n";
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "listen failed: " << WSAGetLastError() << "\n";
        return 1;
    }
    std::cout << "listening on port 5050...\n";

    std::vector<std::thread> workers;
    for (unsigned int i = 0; i < n; i++)
        workers.emplace_back(workerThread);

    std::cout << "workers waiting on completion queue...\n";

    while (true)
    {
        sockaddr_in clientAddr = {};
        int addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &addrLen);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "accept failed: " << WSAGetLastError() << "\n";
            continue;
        }
        std::cout << "client connected! socket=" << clientSocket << "\n";


        int index = allocSession();
        if (index == -1) {
            closesocket(clientSocket);
            continue;
        }
        Session* session = &g_sessionList[index];

        session->socket = clientSocket;

        CreateIoCompletionPort((HANDLE)clientSocket, g_iocp, (ULONG_PTR)session, 0);

        postRecv(session);
    }

    WSACleanup();
    return 0;
}

void postRecv(Session* session)
{
    ZeroMemory(&session->recvOverlapped, sizeof(session->recvOverlapped));
    session->recvWsaBuf.buf = session->recvBuffer.GetWriteBuffer();
    session->recvWsaBuf.len = session->recvBuffer.GetLinearFreeSize();
    
    DWORD flags = 0;
    DWORD byteRecv = 0;

    int ret = WSARecv(
        session->socket,
        &session->recvWsaBuf,
        1,
        &byteRecv,
        &flags,
        &session->recvOverlapped,
        NULL
    );

    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            std::cout << "WSARecv failed: " << err << "\n";
            freeSession(session->index);
            closesocket(session->socket);
        }
    }
}

void flushSend(Session* session) {
    if (session->sendPending) return;
    int used = session->sendBuffer.GetLinearUsedSize();
    if (used == 0) return;

    session->sendPending = true;
    session->sendWsaBuf.buf = session->sendBuffer.GetReadBuffer();
    session->sendWsaBuf.len = used;
    int ret = WSASend(
        session->socket,          
        &session->sendWsaBuf,      
        1,                     
        0,              
        0,                       
        &session->sendOverlapped,  
        NULL                    
    );

    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            std::cout << "WSASend failed: " << err << "\n";
            session->sendPending = false; 
            freeSession(session->index);
            closesocket(session->socket);
        }
    }
}

void postSend(Session* session, const char* data, int len)
{
    session->sendBuffer.Write(data, len);
    std::cout << "Send: " << len << "\n";
    flushSend(session);
}

void initPool() {
    for (int i = 999; i >= 0; i--)
        g_freeIndices.push(i);
}

int allocSession() {
    if (g_freeIndices.empty()) return -1;
    int index = g_freeIndices.top();
    g_freeIndices.pop();
    return index;
}

void freeSession(int index) {
    g_freeIndices.push(index);
}