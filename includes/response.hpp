#pragma once

#include "config/rules/ruleTemplates/locationRule.hpp"
#include "config/types/consts.hpp"
#include "headers.hpp"
#include "server.hpp"
#include "client.hpp"
#include "fd.hpp"

#include <string>

#define DEFAULT_CHUNK_SIZE 4096 // 4 Kb

class Server;

struct ParsedUrl {
    std::string scriptPath;
    std::string pathInfo;
    std::string query;
    bool isValid;
};

class Response {
private:
    HttpStatusCode _statusCode;
    bool _sentHeaders;

protected:
	Request *_request;

public:
    Headers headers;

    static constexpr const char* protocol = "HTTP";
    static constexpr const char* tlsVersion = "1.1";

    Response();
    Response(const Response& other) = default;
    Response& operator=(const Response& other) = default;
    virtual ~Response() = default;

    void setStatusCode(HttpStatusCode code);
    void setStatusCode(StatusCode code);
    void setDefaultHeaders();

	ssize_t sendBodyAsChunk(SocketFD &fd, const std::string &body);
	ssize_t sendBodyAsString(SocketFD &fd, const std::string &body);

    bool headersBeenSent() const;
    void sendHeaders(SocketFD &fd);
    HttpStatusCode getStatusCode() const;

    virtual bool didResponseCreationFail() const;
    virtual HttpStatusCode getFailedResponseStatusCode() const;

    virtual bool isFullResponseSent() const = 0;

    virtual void handleRequestBody(SocketFD &fd, const Request &request) = 0;
    virtual void handleSocketWriteTick(SocketFD &fd) = 0;
    virtual void terminateResponse() = 0;
};

class FileResponse : public Response {
private:
    ReadableFD _fileFD;
    bool _isFinalChunkSent;

public:
    FileResponse(ReadableFD fileFD, Request *request);
    FileResponse(const FileResponse &other) = default;
    FileResponse &operator=(const FileResponse &other) = default;
    ~FileResponse() override;

    bool isFullResponseSent() const override;

    void handleRequestBody(SocketFD &fd, const Request &request) override;
    void handleSocketWriteTick(SocketFD &fd) override;
    void terminateResponse() override;
};

class CGIResponse : public Response {
private:
    enum class CGIResponseTransferMode {
        FullBuffer,
        Chunked,
    };

    Server &_server;
    ReadableFD _cgiOutputFD;
    WritableFD _cgiInputFD;
    
    std::unordered_map<std::string, std::string> _environmentVariables;

    int _timerId;
    int _processId;
    
    bool _chunkedRequestBodyRead;
    bool _hasSentFinalChunk;
    bool _isBrokenBeyondRepair;
    CGIResponseTransferMode _transferMode;
    
    HttpStatusCode _innerStatusCode;
    
    void _setupEnvironmentVariables(const ServerConfig &config, const LocationRule &route, const ParsedUrl &parsedUrl, const Path &serverExecutablePath);

    void _createEnvironmentArray(std::vector<char*> &envPtrs, std::vector<std::string> &strBuff) const;

    void _handleTimeout();
    ssize_t _sendRequestBodyToCGIProcess();
    bool _fetchCGIHeadersFromProcess();
    void _sendCGIResponse();

    void _closeToCGIProcessFd();
    void _closeFromCGIProcessFd();
    
public:
    SocketFD &socketFD;
    Client *client;

    CGIResponse(Server &server, SocketFD &socketFD, Client *client, Request *request);
    CGIResponse(const CGIResponse &other) = delete;
    CGIResponse &operator=(const CGIResponse &other) = delete;
    ~CGIResponse() override;

    bool didResponseCreationFail() const override;
    HttpStatusCode getFailedResponseStatusCode() const override;
    
    void tick();

    void start(const ServerConfig &config, const LocationRule &route, const Path &serverExecutablePath);
    
    void handleRequestBody(SocketFD &fd, const Request &request) override;
    void handleSocketWriteTick(SocketFD &fd) override;
    void terminateResponse() override;

    bool isFullResponseSent() const override;
    bool isBrokenBeyondRepair() const;
};

class StaticResponse : public Response {
private:
    std::string _content;

public:
    StaticResponse(const std::string &content, Request *request);
    StaticResponse(const StaticResponse &other) = default;
    StaticResponse &operator=(const StaticResponse &other) = default;
    ~StaticResponse() override;

    bool isFullResponseSent() const override;
    void handleRequestBody(SocketFD &fd, const Request &request) override;
    void handleSocketWriteTick(SocketFD &fd) override;
    void terminateResponse() override;
};


// std::ostream& operator<<(std::ostream& os, const Response& obj);
// std::ostringstream& operator<<(std::ostringstream& os, const Response& obj);