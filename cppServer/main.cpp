#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <thread>
#include <vector>
#include <iostream>

HANDLE g_iocp;   // 전역 IOCP 핸들 (완료 큐)

struct Session {
    SOCKET socket;
};

enum IoType { IO_RECV, IO_SEND };

struct IoContext {
    OVERLAPPED overlapped;
    WSABUF wsaBuf;
    char buffer[1024];
    IoType type;
};

void postRecv(Session*);
void postSend(Session*, const char*, int);

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
        IoContext* ctx = (IoContext*)overlapped;

        if (!ok || (ctx->type == IO_RECV && bytesTransferred == 0)) {
            closesocket(session->socket);
            std::cout << "[session " << session->socket << "] Disconnect\n";
            delete ctx;
            delete session;
            continue;
        }

        if (ctx->type == IO_RECV) {
            std::cout << "[worker " << std::this_thread::get_id()
                << "] got completion, bytes=" << bytesTransferred << "\n";
            postSend(session, ctx->buffer, bytesTransferred);
            delete ctx;
            postRecv(session);
        }
        else if (ctx->type == IO_SEND){
            std::cout << "[worker] sent " << bytesTransferred << " bytes\n";
            delete ctx;
        }
    }
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    std::cout << "Winsock ready\n";

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

        Session* session = new Session();
        session->socket = clientSocket;

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
    ctx->wsaBuf.buf = ctx->buffer;
    ctx->wsaBuf.len = sizeof(ctx->buffer);

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
            delete ctx;
            delete session;
        }
    }
}

void postSend(Session* session, const char* data, int len)
{
    IoContext* ctx = new IoContext();
    ZeroMemory(&ctx->overlapped, sizeof(ctx->overlapped));

    memcpy(ctx->buffer, data, len);
    ctx->wsaBuf.buf = ctx->buffer;
    ctx->wsaBuf.len = len;

    ctx->type = IO_SEND;

    DWORD bytesSent = 0;
    int ret = WSASend(
        session->socket,
        &ctx->wsaBuf,
        1,
        &bytesSent,
        0,
        &ctx->overlapped,
        NULL
    );

    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            std::cout << "WSASend failed: " << err << "\n";
            closesocket(session->socket);
            delete ctx;
        }
    }
}