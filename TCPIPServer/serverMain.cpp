#include <iostream>
#include <conio.h>//kbhit() to test if theres data in the stdin buffer so that scanf doesnt block while not typing
#include <string>
#include <ws2tcpip.h>
#pragma comment (lib, "ws2_32.lib")//tells the linker to also link with ws2_32.lib

int main()
{
    //init winsock
    WSADATA wsaData;
    WORD requestedVersion = MAKEWORD(2,2);
    if(WSAStartup(requestedVersion, &wsaData))
    {
        std::cerr << "winsock initialization failed! " << WSAGetLastError() << '\n';
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(listenSocket == INVALID_SOCKET)
    {
        std::cerr << "cant create a listening socket! " << WSAGetLastError() << '\n';
        WSACleanup();
        return 1;
    }

    //bind the ip and port to a socket
    sockaddr_in hints;
    hints.sin_family = AF_INET;
    hints.sin_port = htons(54000);
    hints.sin_addr.S_un.S_addr = INADDR_ANY;
    if(bind(listenSocket, reinterpret_cast<sockaddr*>(&hints), sizeof hints) == SOCKET_ERROR)
    {
        std::cerr << "bind failed with error: " << WSAGetLastError() << '\n';
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    
    if(listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "listen failed with error: " << WSAGetLastError() << '\n';
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    //wait for the client to try to establish a connection on the listening socket
    sockaddr_in clientInfo;
    int clientAddressLen = sizeof sockaddr_in;
    SOCKET clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientInfo), &clientAddressLen);
    fd_set socketSet;
    FD_ZERO(&socketSet);
    FD_SET(clientSocket, &socketSet);
    const TIMEVAL selectWaitTime{};
    if(clientSocket == INVALID_SOCKET)
    {
        std::cerr << "accept failed with error: " << WSAGetLastError() << '\n';
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    
    char clientIP[46]{};
    if(inet_ntop(AF_INET, &clientInfo.sin_addr, clientIP, sizeof clientIP)) 
        std::cout << "connected to " << clientIP << '\n';
    else 
        std::cout << "connected successfully\n";

    //no longer need the server's listen socket
    closesocket(listenSocket);

    //message sending loop
    bool connectionOpen = true;
    const int messageBufferLength = 1024;
    char messageBuffer[messageBufferLength];//data comes in as a raw c array of chars
    std::string consoleInput;//but you can use std::string and std::getline() and then call cstr() before sending
    while(connectionOpen)
    {
        //see if the client socket has a message ready for the server to read
        FD_ZERO(&socketSet);
        FD_SET(clientSocket, &socketSet);
        int selectRet = select(0, &socketSet, nullptr, nullptr, &selectWaitTime);
        if(selectRet == SOCKET_ERROR)
        {
            std::cerr << "select failed with error: " << WSAGetLastError() << '\n';
            connectionOpen = false;
        }
        else if(selectRet == 0)//select expired meaning the recv call will block 
        {
            if(_kbhit())
            {
                std::cout << "server: ";
                std::getline(std::cin, consoleInput);

                //size() + 1 so it sends enough bytes to hold the null terminator (which is in a std::string since c++11)
                send(clientSocket, consoleInput.c_str(), consoleInput.size() + 1, 0);
            }
        }
        else//select indicated that recv will not block since it has a message to read from the client
        {
            int recvRet = recv(clientSocket, messageBuffer, messageBufferLength, 0);
            if(recvRet > 0)
            {   
                std::cout << "client: " << messageBuffer << '\n';
            }
            else if(recvRet == 0)
            {
                std::cout << "connection closed\n";
                connectionOpen = false;
            }
            else /*if recvRet < 0*/
            {
                std::cerr << "recv failed with error: " << WSAGetLastError() << '\n';
                connectionOpen = false;
            }
        }
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}