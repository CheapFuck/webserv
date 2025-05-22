#pragma once

#include "config/rules/rules.hpp"
#include "Response.hpp"
#include "Request.hpp"
#include <string>

#include <netinet/in.h>
#include <unordered_map>

class Client {
private:
    int _socket;

    void _handle_get_request(const LocationRule& route);
    void _handle_post_request(const LocationRule& route);
    void _handle_delete_request(const LocationRule& route);

    std::string get_mime_type(const std::string& path) const;

public:
    Client();
    Client(int socket);
    Client(const Client& other);
    Client& operator=(const Client& other);
    ~Client();
    
    bool read_request();
    bool parse_headers();
    void process_request(const ServerConfig& config);
    bool send_response();
    bool is_response_complete() const;
    void cleanup();
    void reset();

    int get_socket() const;

    Request request;
    Response response;
};
