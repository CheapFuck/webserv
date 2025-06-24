#pragma once

#include "baseHandlerObject.hpp"
#include "response.hpp"
#include "request.hpp"
#include "server.hpp"
#include "print.hpp"
#include "CGI.hpp"
#include "fd.hpp"

class CGIClient;
class Server;

class Client : public BaseHandlerObject {
private:
    std::shared_ptr<CGIClient> _cgiClient;
    FD *_CGIWriteFd;
    Server &_server;
    int _serverFd;
    
    std::string _clientIP;
    std::string _clientPort;

    Response &_processRequest(const ServerConfig &config);
    Response &_createCGIClient(const ServerConfig &config, const LocationRule &route);
    Response &_processRequestByMethod(const ServerConfig &config, const LocationRule &route);

public:
    Response response;
    Request request;

    Client(Server &server, int serverFd, const char *clientIP, int clientPort);
    // Client(const Client &other) = default;
    // Client &operator=(const Client &other) = default;
    ~Client() override = default;

    void handleReadCallback(FD &fd, int funcReturnValue) override;
    void handleWriteCallback(FD &fd) override;
    void handleDisconnectCallback(FD &fd) override;

    void handleCGIResponse();

    inline std::string &getClientIP() { return _clientIP; }
    inline std::string &getClientPort() { return _clientPort; }
    inline Server &getServer() { return _server; }
};
