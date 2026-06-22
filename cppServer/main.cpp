#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <thread>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <stack>
#include <atomic>
#include <chrono>
#include"RingBuffer.h"
#include"DoubleBuffer.h"
#include"Player.h"

#pragma pack(push, 1)
struct PacketHeader { unsigned short size; unsigned short id; };

struct MovePacket { PacketHeader h; int playerId; float x, y; };
struct AttackPacket { PacketHeader h; int playerId; float dirX, dirY; int bulletId; };
struct ConnectPacket { PacketHeader h; int playerId; };
#pragma pack(pop)

const int HEADER_SIZE = 4;

struct Session {
    SOCKET socket;
    int index;

    RingBuffer recvBuffer;
    RingBuffer sendBuffer;

    bool sendPending;
    std::mutex sendLock;

    OVERLAPPED recvOverlapped;
    OVERLAPPED sendOverlapped;
    WSABUF recvWsaBuf;
    WSABUF sendWsaBuf;
};

enum class PacketId : unsigned short {
    Connect,
    Disconnect,
    Move,
    Attack,
    Chat,
};

struct RecvPacket {
    int sessionIndex;
    PacketId id;
    std::vector<char> body;
};

constexpr float PLAYER_SPEED = 30.0f;
HANDLE g_iocp;   // IOCP 핸들
Session g_sessionList[1000];
Player g_playerList[1000];
std::stack<int> g_freeIndices;
DoubleBuffer<RecvPacket> g_recvQueue;

void postRecv(Session*);
void postSend(Session*, const char*, int);
void flushSend(Session*);
void initPool();
int allocSession();
void freeSession(int index);
void broadcast(const char*, int);
void handlePacket(RecvPacket&);
void handleMove(RecvPacket&);
void handleConnect(RecvPacket&);


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

                RecvPacket recvPacket;
                recvPacket.sessionIndex = session->index;
                recvPacket.id = static_cast<PacketId>(header.id);
                recvPacket.body.assign(packet + HEADER_SIZE, packet + header.size);

                g_recvQueue.Push(std::move(recvPacket));
            }
            postRecv(session);
        }
        else if (overlapped == &session->sendOverlapped){
            session->sendLock.lock();
            std::cout << "[worker] sent " << bytesTransferred << " bytes\n";
            session->sendBuffer.OnRead(bytesTransferred);
            session->sendPending = false;
            flushSend(session);
            session->sendLock.unlock();
        }
    }
}

void Accepter(SOCKET s) {
    while (true)
    {
        sockaddr_in clientAddr = {};
        int addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(s, (sockaddr*)&clientAddr, &addrLen);
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

        CreateIoCompletionPort((HANDLE)clientSocket, g_iocp, (ULONG_PTR)session, 0);

        RecvPacket rp;
        rp.sessionIndex = index;
        rp.id = PacketId::Connect;
        g_recvQueue.Push(rp);

        postRecv(session);
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

    unsigned int n = std::thread::hardware_concurrency() - 1;
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

    // IOCP 워커 쓰레드(코어 - 1)와 접속 쓰레드 생성
    for (unsigned int i = 0; i < n; i++)
        std::thread(workerThread).detach();
    std::thread (Accepter, listenSocket).detach();

    constexpr int   TICK_MS = 33;    
    constexpr float TICK_DT = TICK_MS / 1000.0f;

    // 메인 루프 시작
    std::vector<RecvPacket> buffer;
    while (true) {
        auto tickStart = std::chrono::steady_clock::now();

        buffer.clear();
        g_recvQueue.Swap(buffer);
        for (auto& packet : buffer) {
            handlePacket(packet);
        }
        std::this_thread::sleep_until(tickStart + std::chrono::milliseconds(TICK_MS));
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

void handlePacket(RecvPacket& packet) {
    switch (packet.id)
    {
    case PacketId::Move:
        handleMove(packet);
        break;
    case PacketId::Connect:
        handleConnect(packet);
        break;
    default:
        break;
    }
}

void handleMove(RecvPacket& packet) {
    packet.body;
}

void handleConnect(RecvPacket& packet) {
    int idx = packet.sessionIndex;
    g_playerList[idx].Init(idx);

    ConnectPacket cp;
    cp.h.size = sizeof(ConnectPacket);
    cp.h.id = static_cast<unsigned short>(PacketId::Connect);
    cp.playerId = idx;

    broadcast((const char*)&cp, sizeof(cp));
}

void broadcast(const char* data, int len) {
    for (int i = 0; i < 1000; i++) {
        Player& target = g_playerList[i];
        if (!target.IsActive()) continue;

        Session* s = &g_sessionList[i];
        s->sendLock.lock();
        s->sendBuffer.Write(data, len);
        flushSend(s);
        s->sendLock.unlock();
    }
}