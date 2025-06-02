#pragma once

#include "config/consts.hpp"
#include "config/rules/rules.hpp"
#include "headers.hpp"

#include <string>

class Response {
private:
    HttpStatusCode _statusCode;
    std::string _body;
    bool _body_set = false;

public:
    Headers headers;

    static constexpr const char* protocol = "HTTP";
    static constexpr const char* tlsVersion = "1.1";

    Response();
    Response(const Response& other);
    Response& operator=(const Response& other);
    ~Response();

    void setStatusCode(HttpStatusCode code);
    void setBody(const std::string& body);
    void setDefaultHeaders();
    void setDefaultBody(ServerConfig& config);

    HttpStatusCode getStatusCode() const;
    const char *getProtocol() const;
    const char *getTlsVersion() const;
    const std::string& getBody() const;

    bool sendToClient(int fd) const;
};

std::ostream& operator<<(std::ostream& os, const Response& obj);
std::ostringstream& operator<<(std::ostringstream& os, const Response& obj);