#pragma once

#include "config/rules/rules.hpp"
#include "baseHandlerObject.hpp"
#include "client.hpp"

class Client;

struct ParsedUrl {
    std::string scriptPath;
    std::string pathInfo;
    std::string query;
    bool isValid;
};

class CGIClient : public BaseHandlerObject, public std::enable_shared_from_this<CGIClient> {
private:
    Client &_client;
    int _processTimerId;
    bool _isRunning;
    int _processTimerId;

    int _toCGIProcessFd;
    int _fromCGIProcessFd;

    std::unordered_map<std::string, std::string> _environmentVariables;

    void _setupEnvironmentVariables(const ServerConfig &config, const LocationRule &route, const ParsedUrl &parsedUrl);
    char * const *_createEnvironmentArray() const;

    void _handleTimeout();

public:
    CGIClient(Client &client);
    CGIClient(const CGIClient &other);
    CGIClient &operator=(const CGIClient &other) = delete;
    ~CGIClient() = default;

    void start(const ServerConfig &config, const LocationRule &route);

    void handleReadCallback(FD &fd, int funcReturnValue) override;
    void handleWriteCallback(FD &fd) override;
    void handleDisconnectCallback(FD &fd) override;

    void exitProcess();

    inline bool isRunning() const { return _isRunning; }
};
