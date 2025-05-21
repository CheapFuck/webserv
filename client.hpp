#pragma once

#include "config/rules/rules.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include <string>

#include <netinet/in.h>
#include <unordered_map>

class Client {
private:
    int _socket;
    std::string _buffer; // Make this a custom buffer class for the request
    std::unordered_map<std::string, std::string> _headers_dict;

public:
    Client();
    Client(int socket);
    Client(const Client& other);
    Client& operator=(const Client& other);
    ~Client();
    
    bool read_request();
    bool is_headers_received();
    bool parse_headers();
    void process_request(const ServerConfig& config);
    bool send_response();
    bool is_response_complete() const;
    const std::string& get_header(const std::string& key) const;
    void cleanup();
    void reset();

    int get_socket() const;
};
