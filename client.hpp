#pragma once

#include "baseHandlerObject.hpp"
#include "response.hpp"
#include "request.hpp"
#include "server.hpp"
#include "print.hpp"
#include "CGI.hpp"
#include "fd.hpp"

class Server;

class Client : public BaseHandlerObject {
private:
    std::shared_ptr<CGIClient> _cgiClient;
    Server &_server;
    int _serverFd;
    
    std::string _clientIP;
    std::string _clientPort;

    Response &_processRequest(const ServerConfig &config);
    Response &_processRequestByMethod(const ServerConfig &config, const LocationRule &route);

public:
    Response response;
    Request request;

    Client(Server &server, int serverFd, std::string &clientIP, std::string &clientPort);
    Client(const Client &other);
    Client &operator=(const Client &other);
    ~Client() override = default;

    void handleReadCallback(FD &fd, int funcReturnValue) override;
    void handleWriteCallback(FD &fd) override;
    void handleDisconnectCallback(FD &fd) override;

    std::string &getClientIP() { return _clientIP; }
    std::string &getClientPort() { return _clientPort; }
};