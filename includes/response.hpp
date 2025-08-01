#pragma once

#include "config/rules/ruleTemplates/locationRule.hpp"
#include "config/types/consts.hpp"
#include "headers.hpp"
#include "server.hpp"
#include "client.hpp"
#include "body.hpp"
#include "fd.hpp"

#include <string>

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
    BodyWriter<FDReader, FDWriter> _bodyWriter;
	Request *_request;
    Client *_client;

public:
    Headers headers;

    static constexpr const char* protocol = "HTTP";
    static constexpr const char* tlsVersion = "1.1";

    Response(Client *client);
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
    virtual bool shouldDirectlySendResponse() const;
    virtual HttpStatusCode getFailedResponseStatusCode() const;

    virtual bool isFullResponseSent() const = 0;

    virtual void handleRequestBody(SocketFD &fd, const Request &request) = 0;
    virtual void handleSocketWriteTick(SocketFD &fd) = 0;
    virtual void terminateResponse() = 0;

	ssize_t fuckyou() { return (_bodyWriter.amountOfBytesWritten); }
};

class FileResponse : public Response {
private:
    ReadableFD _fileFD;
    bool _isFinalChunkSent;

public:
    FileResponse(Client *client, ReadableFD fileFD, Request *request);
    FileResponse(const FileResponse &other) = default;
    FileResponse &operator=(const FileResponse &other) = default;
    ~FileResponse() override;

    bool isFullResponseSent() const override;
    bool shouldDirectlySendResponse() const override;

    void handleRequestBody(SocketFD &fd, const Request &request) override;
    void handleSocketWriteTick(SocketFD &fd) override;
    void terminateResponse() override;
};

class CGIResponse : public Response {
private:
    enum class CGIResponseTransferMode {
        Unknown,
        FullBuffer,
        Chunked,
    };

    Server &_server;
    ReadableFD _cgiOutputFD;
    WritableFD _cgiInputFD;

    std::unordered_map<std::string, std::string> _environmentVariables;
    BodyWriter<FDReader, FDWriter> _pipeWriter;

    ssize_t _sendBytesTracker;
    ssize_t _responseLength;

    int _timerId;
    int _processId;

    bool _chunkedRequestBodyRead;
    bool _hasSentFinalChunk;
    bool _isBrokenBeyondRepair;
    bool _isGamblingResponseWillWork;
    CGIResponseTransferMode _transferMode;

    HttpStatusCode _innerStatusCode;

    void _setupEnvironmentVariables(const ServerConfig &config, const LocationRule &route, const ParsedUrl &parsedUrl, const Path &serverExecutablePath);

    void _createEnvironmentArray(std::vector<char*> &envPtrs, std::vector<std::string> &strBuff) const;

    void _handleCGIInputPipeEvent(WritableFD &fd, short revents);
    void _handleCGIOutputPipeEvent(ReadableFD &fd, short revents);

    void _handleTimeout();
    ssize_t _sendRequestBodyToCGIProcess();
    HttpStatusCode _prepareCGIResponse();
    void _sendCGIResponse();

    void _closeToCGIProcessFd();
    void _closeFromCGIProcessFd();
    
public:
    SocketFD &socketFD;

    CGIResponse(Client *client, Server &server, SocketFD &socketFD, Request *request);
    CGIResponse(const CGIResponse &other) = delete;
    CGIResponse &operator=(const CGIResponse &other) = delete;
    ~CGIResponse() override;

    bool didResponseCreationFail() const override;
    HttpStatusCode getFailedResponseStatusCode() const override;
    
    void tick();

    bool start(const ServerConfig &config, const LocationRule &route, const Path &serverExecutablePath);
    
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
    StaticResponse(Client *client, const std::string &content, Request *request);
    StaticResponse(const StaticResponse &other) = default;
    StaticResponse &operator=(const StaticResponse &other) = default;
    ~StaticResponse() override;

    bool isFullResponseSent() const override;
    bool shouldDirectlySendResponse() const override;

    void handleRequestBody(SocketFD &fd, const Request &request) override;
    void handleSocketWriteTick(SocketFD &fd) override;
    void terminateResponse() override;
};


// std::ostream& operator<<(std::ostream& os, const Response& obj);
// std::ostringstream& operator<<(std::ostringstream& os, const Response& obj);