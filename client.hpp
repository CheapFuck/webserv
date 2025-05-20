#pragma once

#include "config/rules/rules.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include <string>

#include <netinet/in.h>

class Client {
private:
    int _socket;
    std::string _buffer; // Make this a custom buffer class for the request

public:
    Client();
    Client(int socket);
    Client(const Client& other);
    Client& operator=(const Client& other);
    ~Client();
    
    bool read_request();
    bool is_request_complete() const;
    void process_request(const ServerConfig& config);
    bool send_response();
    bool is_response_complete() const;
    void cleanup();
    void reset();

    int get_socket() const;
};
