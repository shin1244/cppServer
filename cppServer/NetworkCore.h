#pragma once
#include <winsock2.h>
#include <mutex>
#include <stack>
#include <iostream>
#include "RingBuffer.h"
#include "DoubleBuffer.h"
#include "Protocol.h"
#pragma comment(lib, "ws2_32.lib")

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

    bool connected;
};

extern HANDLE g_iocp;
extern Session g_sessionList[1000];
extern std::stack<int> g_freeIndices;
extern DoubleBuffer<RecvPacket> g_recvQueue;

void workerThread();
void Accepter(SOCKET listenSocket);
void postRecv(Session*);
void postSend(Session*, const char*, int);
void flushSend(Session*);