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
    request(other.request) {
    std::cout << "Client copy constructor called" << std::endl;
}

Client& Client::operator=(const Client& other) {
    if (this != &other) {
        _socket = other._socket;
        request._buffer = other.request._buffer;
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
        return false;
    }

    while (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        request._buffer.append(buffer, bytes_read);
        if (request.is_headers_received()) break; ;
        bytes_read = recv(_socket, buffer, sizeof(buffer) - 1, 0);
    }
    if (bytes_read < 0) {
        return false;
    }

    return true;
}

void Client::process_request(const ServerConfig& config) {
    // TODO obviously
    (void)config;
    std::cout << "Processing request..." << std::endl;
}

bool Client::send_response() {
    // TODO obviously
	Response response;
	response.setStatusCode(200);
	response.setHeader("Content-Length", "0");
	response.setBody("");
    ssize_t bytes_sent = send(_socket, response.toString().c_str(), response.toString().length(), 0);
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
    request._buffer.clear();
    // Delete unordered map as well
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
