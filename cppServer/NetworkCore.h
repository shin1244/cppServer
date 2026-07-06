#pragma once
#include <winsock2.h>
#include <mutex>
#include <stack>
#include <iostream>
#include "RingBuffer.h"
#include "DoubleBuffer.h"
#include "EventQueue.h"
#include "Protocol.h"
#include "ObjectPool.h"
#pragma comment(lib, "ws2_32.lib")

#define USE_EVENT_QUEUE

struct Session {
    SOCKET socket;
    int index;
    int roomId = -1;

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
extern ObjectPool<Session, 1000> g_sessions;
extern std::stack<int> g_freeIndices;

#ifdef USE_EVENT_QUEUE
    extern EventQueue<RecvPacket> g_recvQueue;
#else
    extern DoubleBuffer<RecvPacket> g_recvQueue;
#endif

void workerThread();
void Accepter(SOCKET listenSocket);
void postRecv(Session*);
void postSend(Session*, const char*, int);
void flushSend(Session*);