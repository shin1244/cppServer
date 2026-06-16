#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#include <iostream>

int main()
{
    WSADATA wsa;


    int r = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (r != 0) {                                                   
        std::cout << "WSAStartup failed: " << r << "\n";            
        return 1;                                                   
    }                                                               
                                                                    
    std::cout << "Winsock ready\n";                                 
                                                                    
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cout << "socket failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }
    std::cout << "socket created\n";

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(5050);

    if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) 
    {
        std::cout << "bind failed: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "bind ok\n";

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cout << "listen failed: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "listening on port 5050...\n";

    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &addrLen);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "accept failed: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "client connected!\n";

    char buffer[1024] = {};
    int recvLen = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (recvLen > 0) {
        std::cout << "received (" << recvLen << " bytes): " << buffer << "\n";
        send(clientSocket, buffer, recvLen, 0);
    }

    closesocket(listenSocket);
    WSACleanup();
    return 0;
}