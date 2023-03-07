#include <iostream>
#include <conio.h>//kbhit() to test if theres data in the stdin buffer so that scanf doesnt block while not typing
#include <string>
#include <ws2tcpip.h>

//Need to link with Ws2_32.lib, Mswsock.lib, and AdvApi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

int main()
{
    //init winsock
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2), &wsaData))
    {
        std::cerr << "WSAStartup failed with error: " << WSAGetLastError() << '\n';
        return 1;
    }

    //create a socket for connecting to the server
    SOCKET connectionSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(connectionSocket == INVALID_SOCKET)
    {
        std::cerr << "unable to connect to server!\n";
        WSACleanup();
        return 1;
    }

    //ask user for the servers ip address
    std::string serverAddressFromConsole;
    sockaddr_in serverAddr{.sin_family = AF_INET, .sin_port = htons(54000)};
    for(bool isAddressInvalid = true; isAddressInvalid;)
    { 
        std::cout << "enter an ip address to connect to: ";
        std::getline(std::cin, serverAddressFromConsole);
        int ptonRet = inet_pton(AF_INET, serverAddressFromConsole.c_str(), &serverAddr.sin_addr.S_un.S_addr);
        if(ptonRet == 0)
        {
            std::cerr << "the address you entered is not a valid IPv4 dotted-decimal string\n"
                      << "a correct IPv4 example: 147.202.234.120\n";
        }
        else if(ptonRet < 0)
        {
            std::cerr << "inet_pton failed with error: " << WSAGetLastError() << '\n';
            WSACleanup();
            closesocket(connectionSocket);
            return 1;
        }
        else isAddressInvalid = false;
    }
    
    //try to connect to the server
    std::cout << "trying to connect to " << serverAddressFromConsole << '\n';
    if(connect(connectionSocket, reinterpret_cast<sockaddr*>(&serverAddr),
        static_cast<int>(sizeof serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "connect failed with error: " << WSAGetLastError() << '\n';
        closesocket(connectionSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "connected to " << serverAddressFromConsole << '\n';

    const int recvBuffLen = 1024;
    char recvbuff[recvBuffLen];
    std::string consoleInput;
    fd_set socketSet;
    FD_ZERO(&socketSet);
    FD_SET(connectionSocket, &socketSet);
    TIMEVAL selectWaitTime{};
    bool connectionOpen = true;
    while(connectionOpen)
    {
        //see if the server socket has a message for us to recv
        FD_ZERO(&socketSet);
        FD_SET(connectionSocket, &socketSet);
        int selectRet = select(0, &socketSet, nullptr, nullptr, &selectWaitTime);
        if(selectRet == SOCKET_ERROR)
        {
            std::cerr << "select failed with error: " << WSAGetLastError() << '\n';
            connectionOpen = false;
        }
        else if(selectRet > 0)//if the server has a message for us ready to be read from connectionSocket
        {
            int recvRet = recv(connectionSocket, recvbuff, recvBuffLen, 0);
            if(recvRet > 0)
            {
                std::cout << "server: " << recvbuff << '\n';
            }
            else if(recvRet == 0)
            {
                std::cout << "connection closed\n";
            }
            else
            {
                std::cerr << "recv failed with error: " << WSAGetLastError() << '\n';
                connectionOpen = false;
            }
        }
        else//if selectRet == 0 then there is no message yet from the server so keep sending data
        {
            //if there is data in the stdin buffer from the console to read
            if(_kbhit())
            {     
                std::cout << "client: ";
                std::getline(std::cin, consoleInput);

                //size() + 1 so it sends enough bytes to hold the null terminator (which is in a std::string since c++11)
                send(connectionSocket, consoleInput.c_str(), consoleInput.size() + 1, 0);
            }
        }
    }

    closesocket(connectionSocket);
    WSACleanup();
    return 0;
}