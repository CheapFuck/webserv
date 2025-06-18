#pragma once

#include "baseHandlerObject.hpp"
#include "response.hpp"
#include "request.hpp"
#include "server.hpp"
#include "print.hpp"
#include "fd.hpp"

class Server;

class Client : public BaseHandlerObject {
private:
    Server &_server;
    int _serverFd;

    Response &_processRequest(const ServerConfig &config);
    Response &_processRequestByMethod(const ServerConfig &config, const LocationRule &route);

public:
    Response response;
    Request request;

    Client(Server &server, int serverFd);
    Client(const Client &other);
    Client &operator=(const Client &other);
    ~Client() override = default;

    void handleReadCallback(FD &fd, int funcReturnValue) override;
    void handleWriteCallback(FD &fd) override;
    void handleDisconnectCallback(FD &fd) override;
};