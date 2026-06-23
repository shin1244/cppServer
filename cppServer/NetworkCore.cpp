#include"NetworkCore.h"
#include"Protocol.h"

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


            std::cout << "[session " << session->socket << "] Disconnect\n";
            continue;
        }

        if (overlapped == &session->recvOverlapped) {
            if (bytesTransferred == 0) {
                RecvPacket recvPacket;
                recvPacket.sessionIndex = session->index;
                recvPacket.id = PacketId::Disconnect;
                g_recvQueue.Push(std::move(recvPacket));

                continue;
            }
            session->recvBuffer.OnWrite(bytesTransferred);

            while (true) {
                if (session->recvBuffer.GetUsedSize() < HEADER_SIZE) break;

                PacketHeader header;
                session->recvBuffer.Peek((char*)&header, HEADER_SIZE);

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
        else if (overlapped == &session->sendOverlapped) {
            session->sendLock.lock();
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