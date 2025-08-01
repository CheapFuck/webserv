#pragma once

#include "response.hpp"
#include "request.hpp"
#include "server.hpp"
#include "print.hpp"
#include "fd.hpp"

// class CGIClient;
class Server;
class Response;

enum class ClientHTTPState {
    Idle,
    WaitingForHeaders,
    ReadingBody,
    SendingResponse,
};

class Client {
private:
    Server &_server;
    int _serverFd;
    ClientHTTPState _state;

    bool _chunkedRequestBodyRead;
    bool _isFirstRequest;

    std::string _clientIP;
    std::string _clientPort;

    Response &_processRequest(const ServerConfig &config);
    Response &_handleReturnRuleResponse(const ReturnRule &returnRule);
    Response &_createCGIClient(const ServerConfig &config, const LocationRule &route);
    Response &_processRequestByMethod(const ServerConfig &config, const LocationRule &route);

    Response *_configureResponse(Response *response, HttpStatusCode statusCode = HttpStatusCode::OK);
    Response *_createErrorResponse(HttpStatusCode statusCode, const LocationRule &route, bool shouldCloseConnection = true);
    Response *_createDirectoryListingResponse(const LocationRule &route);
    Response *_createReturnRuleResponse(const ReturnRule &returnRule);
    Response *_createCGIResponse(SocketFD &fd, const ServerConfig &config, const LocationRule &route);
    Response *_createResponseFromRequest(SocketFD &fd, Request &request);

public:
    const LocationRule *route;
    Response *response;
    Request request;

    Client(Server &server, int serverFd, const char *clientIP, int clientPort);
    Client &operator=(const Client &other) = delete;
    Client(const Client &other) = delete;
    ~Client();

    void handleRead(SocketFD &fd, ssize_t funcReturnValue);
    void handleWrite(SocketFD &fd);
    void handleClientReset(SocketFD &fd);
    void switchResponseToErrorResponse(HttpStatusCode statusCode, SocketFD &fd);

    bool isFullRequestBodyReceived(SocketFD &fd) const;
    bool isTimedOut(const HTTPRule &httpRule, const SocketFD &fd) const;
    bool shouldBeClosed(const HTTPRule &httpRule, const SocketFD &fd) const;
    bool setEpollWriteNotification(SocketFD &fd);
    bool unsetEpollWriteNotification(SocketFD &fd);
    ClientHTTPState getState() const;

    inline std::string &getClientIP() { return _clientIP; }
    inline std::string &getClientPort() { return _clientPort; }
    inline Server &getServer() { return _server; }
};
