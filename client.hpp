#pragma once

#include "config/rules/rules.hpp"
#include "response.hpp"
#include "request.hpp"
#include <string>

#include <netinet/in.h>
#include <unordered_map>

class Client {
private:
    Response &_processRequestByMethod(const ServerConfig &config, const LocationRule &route);

public:
    Request request;
    Response response;
    
    Client() = default;
    Client(const Client &other);
    Client &operator=(const Client &other);
    ~Client() = default;

    bool read(int fd);
    bool sendResponse(int fd, ServerConfig& using_conf);
    bool requestIsComplete() const;
    Response &processRequest(const ServerConfig &config);

    void reset();
};
