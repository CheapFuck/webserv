#include "client.hpp"

#include <iostream>
#include <sstream>
#include <unistd.h>

Client::Client(int socket) :
    _socket(socket) {
    std::cout << "Client created with socket: " << socket << std::endl;
}

Client::Client(const Client& other) :
    _socket(other._socket),
    _buffer(other._buffer) {
    std::cout << "Client copy constructor called" << std::endl;
}

Client& Client::operator=(const Client& other) {
    if (this != &other) {
        _socket = other._socket;
        _buffer = other._buffer;
        std::cout << "Client assignment operator called" << std::endl;
    }
    return *this;
}

Client::~Client() {}

Client::Client() : _socket(-1) {
    std::cout << "Client created with default socket" << std::endl;
}

bool Client::read_request() {
    char buffer[2];
    ssize_t bytes_read = recv(_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read == 0) {
        // Client disconnected
        return false;
    }

    while (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        _buffer.append(buffer, bytes_read);
        if (is_headers_received()) break; ;
        bytes_read = recv(_socket, buffer, sizeof(buffer) - 1, 0);
    }
    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            exit(0);
        // Potentially an error - but ye well 42 rules yk...
        return false;
    }

    return true;
}

const std::string& Client::get_header(const std::string& key) const
{
    std::unordered_map<std::string, std::string>::const_iterator iter = _headers_dict.find(key);
    if (iter == _headers_dict.end())
        throw std::out_of_range("No such header was parsed: " + key);
    return (iter->second);
}

bool Client::parse_headers() 
{
    std::istringstream stream(_buffer);
    std::string line;
    while (std::getline(stream, line, '\n')) 
    {
        // Remove trailing \r
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // Find position of colon
        size_t colon = line.find(':');
        if (colon != std::string::npos) 
        {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            _headers_dict[key] = value;
        }   
    }

    try
    {
        std::cout << get_header("Host") << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return (true);
}

bool Client::is_headers_received() {
    if (_buffer.find("\r\n\r\n") == std::string::npos) {
        return false;
    }
    parse_headers();
    return true;
}

void Client::process_request(const ServerConfig& config) {
    // TODO obviously
    (void)config;
    std::cout << "Processing request..." << std::endl;
}

bool Client::send_response() {
    // TODO obviously
    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    ssize_t bytes_sent = send(_socket, response.c_str(), response.size(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Failed to send response" << std::endl;
        return false;
    }
    std::cout << "Sent response: " << response << std::endl;
    return true;
}

bool Client::is_response_complete() const {
    return true;
}

void Client::reset() {
    _buffer.clear();
    std::cout << "Client reset" << std::endl;
}

int Client::get_socket() const {
    return _socket;
}

void Client::cleanup() {
    if (_socket != -1) {
        close(_socket);
        _socket = -1;
    }
}
