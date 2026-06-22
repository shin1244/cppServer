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
#include"Protocol.h"
#include"NetworkCore.h"

constexpr float PLAYER_SPEED = 30.0f;
HANDLE g_iocp;   // IOCP 핸들
Session g_sessionList[1000];
Player g_playerList[1000];
std::stack<int> g_freeIndices;
DoubleBuffer<RecvPacket> g_recvQueue;


void broadcast(const char*, int);
void handlePacket(RecvPacket&);
void handleMove(RecvPacket&);
void handleConnect(RecvPacket&);


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