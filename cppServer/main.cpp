#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <thread>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <stack>
#include <atomic>

class RingBuffer;

const int HEADER_SIZE = 2;

struct Session {
    SOCKET socket;
    int id;
    int index;

    RingBuffer recvBuffer;
    RingBuffer sendBuffer;

    bool sendPending;

    OVERLAPPED recvOverlapped;
    OVERLAPPED sendOverlapped;
    WSABUF recvWsaBuf;
    WSABUF sendWsaBuf;
};

enum IoType { IO_RECV, IO_SEND };

HANDLE g_iocp;   // IOCP 핸들
Session g_sessionList[1000];
std::unordered_map<int, Session*> g_sessionMap;
std::stack<int> g_freeIndices;
std::atomic<int> g_nextId{ 100 };
int g_sessionCount = 0;

void postRecv(Session*);
void postSend(Session*, const char*, int);
void initPool();
int allocSession();
void freeSession(int);

class RingBuffer {
private:
    char buffer[4096];
    int head = 0;
    int tail = 0;
public:
    // 현재 버퍼에 쌓인 데이터의 크기를 반환합니다. 
    int GetUsedSize() { return (tail - head + 4096) % 4096; }
    // 현재 버퍼에 빈 공간을 반환합니다.
    int GetFreeSize() { return 4096 - GetUsedSize() - 1; }
    // 현재 버퍼에 연속된 빈 공간을 반환합니다.
    int GetLinearFreeSize() {
        if (tail >= head) {
            return 4096 - tail;
        } 
        else {
            return head - tail - 1;
        }
    }
    int GetLinearFreeSize() {
        if (head < tail) {
            return tail - head;
        } 
        else {
            return 4096 - head;
        }
    }
    // 버퍼를 초기화 합니다.
    void Clear() { head = tail = 0; }
    // 현재 데이터를 새로 적을 수 있는 빈 공간의 시작 주소를 반환합니다.
    char* GetWriteBuffer() { return &buffer[tail]; }
    // OS가 네트워크 카드에서 데이터를 받아 버퍼에 성공적으로 채웠을 때 tail을 전진시킵니다.
    void OnWrite(int bytes) { tail = (tail + bytes) % 4096; }
    // 현재 읽을 수 있는 가장 오래된 데이터의 시작 주소를 반환합니다.
    char* GetReadBuffer() { return &buffer[head]; }
    // 메인 스레드가 패킷 하나를 온전히 조립해서 로직에 반영했다면 head를 전진시킵니다.
    void OnRead(int bytes) { head = (head + bytes) % 4096; }

    void Peek(char* dest, int len) {
        if (GetUsedSize() < len) return;
        if (tail >= head) {
            memcpy(dest, &buffer[head], len);
        }
        else {
            int rightSize = 4096 - head;
            if (len <= rightSize) {
                memcpy(dest, &buffer[head], len);
            }
            else {
                memcpy(dest, &buffer[head], rightSize);
                memcpy(dest + rightSize, &buffer[0], len - rightSize);
            }
        }
    }

    bool Write(const char* data, int len) {
        if (len > GetFreeSize()) return false;

        int rightSize = 4096 - tail;
        if (len <= rightSize) {
            memcpy(&buffer[tail], data, len);
        }
        else {
            memcpy(&buffer[tail], data, rightSize);
            memcpy(&buffer[0], data + rightSize, len - rightSize);
        }
        OnWrite(len);
        return true;
    }

};

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
            closesocket(session->socket);
            std::cout << "[session " << session->socket << "] Disconnect\n";

            g_sessionMap.erase(session->id);
            freeSession(session->index);
            continue;
        }

        if (overlapped == &session->recvOverlapped) {
            session->recvBuffer.OnWrite(bytesTransferred);

            while (true) {
                if (session->recvBuffer.GetUsedSize() < HEADER_SIZE) break;

                char h[HEADER_SIZE];
                session->recvBuffer.Peek(&h, HEADER_SIZE);

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
        session->index = index;
        session->id = g_nextId++;

        g_sessionMap[session->id] = session;

        CreateIoCompletionPort((HANDLE)clientSocket, g_iocp, (ULONG_PTR)session, 0);

        postRecv(session);
    }

    WSACleanup();
    return 0;
}

void postRecv(Session* session)
{
    IoContext* ctx = new IoContext();
    ZeroMemory(&ctx->overlapped, sizeof(ctx->overlapped));
    ctx->wsaBuf.buf = session->recvBuffer.GetWriteBuffer();
    ctx->wsaBuf.len = session->recvBuffer.GetFreeSize();


    DWORD flags = 0;
    DWORD byteRecv = 0;

    int ret = WSARecv(
        session->socket,
        &ctx->wsaBuf,
        1,
        &byteRecv,
        &flags,
        &ctx->overlapped,
        NULL
    );

    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            std::cout << "WSARecv failed: " << err << "\n";
            closesocket(session->socket);
            g_sessionMap.erase(session->id);
            freeSession(session->index);
            delete ctx;
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
            closesocket(session->socket);
        }
    }
}

void postSend(Session* session, const char* data, int len)
{
    session->sendBuffer.Write(data, len);
    flushSend(session);
}

void initPool() {
    for (int i = 0; i < 1000; i++) {
        g_freeIndices.push(i);
    }
}

int allocSession() {
    if (g_sessionCount >= 1000) return -1;
    return g_sessionCount++;
}

void freeSession(int index) {
    g_sessionMap.erase(g_sessionList[index].id);
    g_sessionCount--;
    if (index != g_sessionCount) {
        g_sessionList[index] = g_sessionList[g_sessionCount];
        g_sessionList[index].index = index;
        g_sessionMap[g_sessionList[index].id] = &g_sessionList[index];
    }
}