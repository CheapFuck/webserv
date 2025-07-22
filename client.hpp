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
    WaitingForHeaders,
    ReadingBody,
    CopyingBody,
    TrashingBody,
    SwitchingToOutput,
    SendingResponse,
};

class Client : public std::enable_shared_from_this<Client> {
private:
    Server &_server;
    int _serverFd;
    ClientHTTPState _state;

    std::string _clientIP;
    std::string _clientPort;

    std::unique_ptr<Response> _processRequest(const ServerConfig &config);
    std::unique_ptr<Response> _handleReturnRuleResponse(const ReturnRule &returnRule);
    std::unique_ptr<Response> _createCGIClient(const ServerConfig &config, const LocationRule &route);
    std::unique_ptr<Response> _processRequestByMethod(const ServerConfig &config, const LocationRule &route);

    // std::unique_ptr<Response> _configureResponse(Response *response, HttpStatusCode statusCode = HttpStatusCode::OK);
    std::unique_ptr<Response> _configureResponse(std::unique_ptr<Response> response, HttpStatusCode statusCode = HttpStatusCode::OK);
    std::unique_ptr<Response> _createErrorResponse(HttpStatusCode statusCode, const LocationRule &route);
    std::unique_ptr<Response> _createDirectoryListingResponse(const LocationRule &route);
    std::unique_ptr<Response> _createReturnRuleResponse(const ReturnRule &returnRule);
    std::unique_ptr<Response> _createCGIResponse(SocketFD &fd, const ServerConfig &config, const LocationRule &route);
    std::unique_ptr<Response> _createResponseFromRequest(SocketFD &fd, Request &request);    

public:
    std::unique_ptr<Response> response;
    Request request;

    Client(Server &server, int serverFd, const char *clientIP, int clientPort);
    ~Client();
    // Client(const Client &other) = default;
    // Client &operator=(const Client &other) = default;

    void handleRead(SocketFD &fd, ssize_t funcReturnValue);
    void handleWrite(SocketFD &fd);
    void switchResponseToErrorResponse(HttpStatusCode statusCode);

    inline std::string &getClientIP() { return _clientIP; }
    inline std::string &getClientPort() { return _clientPort; }
    inline Server &getServer() { return _server; }
};
