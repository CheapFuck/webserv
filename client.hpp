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
    bool _try_create_response_from_file(const Path& filepath);
    bool _try_to_create_response_from_index(const Path& base_filepath, const IndexRule& index_rule);

    std::string get_mime_type(const std::string& path) const;

public:
    Client();
    Client(int socket, const ServerConfig& config);
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

struct FilePart {
    std::string name;
    std::string filename;
    std::string contentType;
    std::string content;
};
