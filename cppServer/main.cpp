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
#include"Bullet.h"

HANDLE g_iocp;   // IOCP 핸들
Session g_sessionList[1000];
Player g_playerList[1000];
Bullet g_bulletList[1000];
std::stack<int> g_freeIndices;
DoubleBuffer<RecvPacket> g_recvQueue;


void broadcast(const char*, int);
void handlePacket(RecvPacket&);

void handleConnect(RecvPacket&);
void handleDisconnect(RecvPacket&);
void handleMove(RecvPacket&);
void handleAttack(RecvPacket&);

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

        for (int i = 0; i < 1000; i++) {
            if (!g_playerList[i].IsActive()) continue;
            g_playerList[i].Update(TICK_DT);
        }

        for (int i = 0; i < 1000; i++) {
            if (!g_bulletList[i].IsActive()) continue;
            g_bulletList[i].Update(TICK_DT);
        }

        for (int i = 0; i < 1000; i++) {
            if (!g_playerList[i].IsActive()) continue;

            for (int j = 0; j < 1000; j++) {
                if (!g_bulletList[j].IsActive()) continue;
                if (i == g_bulletList[j].GetOwnerId()) continue;

                float dx = g_playerList[i].GetX() - g_bulletList[j].GetX();
                float dy = g_playerList[i].GetY() - g_bulletList[j].GetY();

                float hitRadius = 14.0f;
                float distSq = dx * dx + dy * dy;

                if (distSq <= hitRadius * hitRadius) {
                    std::cout << "KILL!!" << "\n";
                    g_bulletList[j].Clear();
                }
            }
        }

        for (int i = 0; i < 1000; i++) {
            if (!g_playerList[i].IsActive()) continue;
            MovePacket mp;
            mp.h.size = sizeof(MovePacket);
            mp.h.id = static_cast<unsigned short>(PacketId::Move);
            mp.playerId = i;
            mp.x = g_playerList[i].GetX();
            mp.y = g_playerList[i].GetY();
            broadcast((const char*)&mp, sizeof(mp));
        }

        for (int i = 0; i < 1000; i++) {
            if (!g_bulletList[i].IsActive()) continue;
            BulletMovePacket bp;
            bp.h.size = sizeof(BulletMovePacket);
            bp.h.id = static_cast<unsigned short>(PacketId::Attack);
            bp.bulletId = i;
            bp.ownerId = g_bulletList[i].GetOwnerId();
            bp.x = g_bulletList[i].GetX();
            bp.y = g_bulletList[i].GetY();
            broadcast((const char*)&bp, sizeof(bp));
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
    case PacketId::Disconnect:
        handleDisconnect(packet);
        break;
    case PacketId::Attack:
        handleAttack(packet);
    default:
        break;
    }
}

void handleMove(RecvPacket& packet) {
    if (packet.body.size() < 1) return;
    unsigned char keys = static_cast<unsigned char>(packet.body[0]);
    g_playerList[packet.sessionIndex].SetKeys(keys);
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

void handleDisconnect(RecvPacket& packet) {
    int idx = packet.sessionIndex;

    closesocket(g_sessionList[idx].socket);
    g_playerList[idx].Clear();

    DisconnectPacket dp;
    dp.h.size = sizeof(DisconnectPacket);
    dp.h.id = static_cast<unsigned short>(PacketId::Disconnect);
    dp.playerId = idx;

    broadcast((const char*)&dp, sizeof(dp));

    freeSession(idx);
}

void handleAttack(RecvPacket& packet) {
    if (packet.body.size() < sizeof(float) * 2) return;

    float dirX;
    float dirY;
     
    memcpy(&dirX, packet.body.data(), sizeof(float));
    memcpy(&dirY, packet.body.data() + sizeof(float), sizeof(float));

    int ownerId = packet.sessionIndex;

    for (int i = 0; i < 1000; i++) {
        if (g_bulletList[i].IsActive()) continue;

        g_bulletList[i].Fire(
            ownerId,
            g_playerList[ownerId].GetX(),
            g_playerList[ownerId].GetY(),
            dirX,
            dirY
        );

        std::cout << "Bullet fired owner=" << ownerId
            << " bullet=" << i
            << " dir=(" << dirX << ", " << dirY << ")\n";
        break;
    }
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