#include "SocketServer.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>

SocketServer::SocketServer(int port)
    : _serverSocket(-1), _port(port), _running(false)
{
}

SocketServer::~SocketServer()
{
    Stop();
}

void SocketServer::HandleClient(int clientSocket)
{
    char buffer[4096];

    int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);

    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        std::string request(buffer);
        
        std::cout << "Received: " << request;
        
        std::string response = "ERROR|No handler set\n";
        
        if (_requestHandler)
            response = _requestHandler(request);
        
        write(clientSocket, response.c_str(), response.length());
    }

    close(clientSocket);
    std::cout << "Client disconnected!!!\n";
}

void SocketServer::SetRequestHandler(std::function<std::string(std::string const &)> handler)
{
    _requestHandler = handler;
}

void SocketServer::Start()
{
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket < 0)
    {
        std::cout << "Failed to create socket\n";
        return;
    }

    int opt = 1;
    setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(_port);

    if (bind(_serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cout << "Failed to bind to port " << _port << "\n";
        close(_serverSocket);
        return;
    }
    
    if (listen(_serverSocket, 10) < 0)
    {
        std::cout << "Failed to listen\n";
        close(_serverSocket);
        return;
    }

     _running = true;
    std::cout << "Server listening on port " << _port << "\n";

    while (_running)
    {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        int clientSocket = accept(_serverSocket, (sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket < 0)
        {
            if (_running)
                std::cout << "Failed to accept client\n";
            continue;
        }
        std::cout << "Client connected\n";
        
        std::thread(&SocketServer::HandleClient, this, clientSocket).detach();
    }
}

void SocketServer::Stop()
{
    _running = false;
    if (_serverSocket >= 0)
    {
        close(_serverSocket);
        _serverSocket = -1;
    }
}
