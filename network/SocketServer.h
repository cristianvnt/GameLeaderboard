#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <string>
#include <functional>
#include <map>

class SocketServer
{
private:
    int _serverSocket;
    int _port;
    bool _running;
    
    std::function<std::string(std::string const&)> _requestHandler;
    
    void HandleClient(int clientSocket);
    
public:
    SocketServer(int port);
    ~SocketServer();
    
    void SetRequestHandler(std::function<std::string(std::string const&)> handler);
    void Start();
    void Stop();
};

#endif