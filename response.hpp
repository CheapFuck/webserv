#pragma once

#include "config/rules/ruleTemplates/locationRule.hpp"
#include "config/types/consts.hpp"
#include "headers.hpp"
#include "server.hpp"
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

    bool shouldSendHeaders() const;
    bool didResponseCreationFail() const;

    void sendHeaders(SocketFD &fd);
    HttpStatusCode getStatusCode() const;
    HttpStatusCode getFailedResponseStatusCode() const;

    virtual bool isFullRequestBodyRecieved() const = 0;
    virtual bool isFullResponseSent() const = 0;

    virtual void handleRequestBody(SocketFD &fd) = 0;
    virtual void handleSocketWriteTick(SocketFD &fd) = 0;
    virtual void terminateResponse() = 0;
};

class FileResponse : public Response {
private:
    ReadableFD _fileFD;

public:
    FileResponse(ReadableFD fileFD);
    FileResponse(const FileResponse &other) = default;
    FileResponse &operator=(const FileResponse &other) = default;
    ~FileResponse() override;

    bool isFullRequestBodyRecieved() const override;
    bool isFullResponseSent() const override;

    void handleRequestBody(SocketFD &fd) override;
    void handleSocketWriteTick(SocketFD &fd) override;
    void terminateResponse() override;
};

// class CGIResponse : public Response {
// private:
//     Server &_server;
//     std::shared_ptr<Client> _client;
//     SocketFD &_socketFD;
//     ReadableFD _cgiOutputFD;
//     WritableFD _cgiInputFD;

//     std::unordered_map<std::string, std::string> _environmentVariables;

//     int _timerId;
//     int _processTimerId;

//     bool _chunkedResponseDone = false;

//     HttpStatusCode _innerStatusCode;

//     void _setupEnvironmentVariables(const ServerConfig &config, const LocationRule &route, const ParsedUrl &parsedUrl);

//     char * const *_createEnvironmentArray() const;

//     void _handleTimeout();
//     void _handleCGIInputCallback(WritableFD &fd, short revents);
//     void _handleCGIOutputCallback(ReadableFD &fd, short revents);

// public:
//     CGIResponse(Server &server, SocketFD &socketFD, std::shared_ptr<Client> client);
//     CGIResponse(const CGIResponse &other) = default;
//     CGIResponse &operator=(const CGIResponse &other) = default;
//     ~CGIResponse() override;

//     bool didResponseCreationFail() const override;
//     HttpStatusCode getFailedResponseStatusCode() const override;

//     void start(const ServerConfig &config, const LocationRule &route);

//     void handleRequestBody(SocketFD &fd) override;
//     void handleSocketWriteTick(SocketFD &fd) override;

//     void updateFromCGIOutput(const std::string &cgiOutput);
// };

class StaticResponse : public Response {
private:
    std::string _content;

public:
    StaticResponse(const std::string &content);
    StaticResponse(const StaticResponse &other) = default;
    StaticResponse &operator=(const StaticResponse &other) = default;
    ~StaticResponse() override;

    bool isFullRequestBodyRecieved() const override;
    bool isFullResponseSent() const override;
    void handleRequestBody(SocketFD &fd) override;
    void handleSocketWriteTick(SocketFD &fd) override;
    void terminateResponse() override;
};


// std::ostream& operator<<(std::ostream& os, const Response& obj);
// std::ostringstream& operator<<(std::ostringstream& os, const Response& obj);