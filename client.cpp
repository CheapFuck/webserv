#include "client.hpp"
#include "Response.hpp"

#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <fstream>
#include <sstream>

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
        request = other.request;
        std::cout << "Client assignment operator called" << std::endl;
    }
    return *this;
}

Client::~Client() {}

Client::Client() : _socket(-1) {
    std::cout << "Client created with default socket" << std::endl;
}

std::string Client::get_mime_type(const std::string& path) const {
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".js")) return "application/javascript";
    if (path.ends_with(".png")) return "image/png";
    if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
    if (path.ends_with(".gif")) return "image/gif";
    return "application/octet-stream";
}

bool Client::read_request() {
    char buffer[64];
    ssize_t bytes_read = recv(_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read == 0) {
        return false;
    }

    while (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        request.append_data(std::string(buffer));
        bytes_read = recv(_socket, buffer, sizeof(buffer) - 1, 0);
    }
    if (bytes_read < 0) {
        return true;
    }

    return true;
}

void Client::_handle_get_request(const LocationRule& route) {
    std::cout << "Handling GET request" << std::endl;
    std::cout << route << std::endl;
    if (!route.root.is_set()) {
        std::cerr << "Root not set" << std::endl;
        return;
    }

    std::string rootDir = route.root.get().get_path();
    std::string request_path = std::string(request.get_path()).replace(0, route.get_path().length(), rootDir);

    if (request_path.find("..") != std::string::npos) {
        std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        send(_socket, response.c_str(), response.size(), 0);
    }

    struct stat path_stat{};
    if (stat(request_path.c_str(), &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
        std::cout << "Request path is a directory" << std::endl;
        if (route.index.is_set()) {
            request_path = request_path + "/" + route.index.get();
            std::cout << "Request path: " << request_path << std::endl;
        } else {
            std::string response = "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n";
            send(_socket, response.c_str(), response.size(), 0);
            return;
        }
    }

    std::cout << "Request path: " << request_path << std::endl;
    std::ifstream file(request_path, std::ios::binary);
    if (!file) {
        std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(_socket, response.c_str(), response.size(), 0);
        return;
    } else {
        std::ostringstream ss;
        ss << file.rdbuf();
        file.close();

        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: " + get_mime_type(request_path) + "\r\n";
        response += "Content-Length: " + std::to_string(ss.str().size()) + "\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += ss.str();
        send(_socket, response.c_str(), response.size(), 0);
    } 
}

void Client::_handle_post_request(const LocationRule& route) {
    (void)route;
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: 0\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    send(_socket, response.c_str(), response.size(), 0);
}

void Client::_handle_delete_request(const LocationRule& route) {
    (void)route;
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: 0\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    send(_socket, response.c_str(), response.size(), 0);
}

void Client::process_request(const ServerConfig& config) {
    (void)config;
    return ;
    const LocationRule *route = config.routes.find(request.get_path());
    if (route == nullptr) {
        std::string response = "HTTP/1.1 404 Route Not Found\r\nContent-Length: 0\r\n\r\n";
        send(_socket, response.c_str(), response.size(), 0);
    }

    if (!route->methods.is_allowed(request.get_method())) {
        std::string response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
        send(_socket, response.c_str(), response.size(), 0);
    }

    if (route->redirect.is_set()) {
        std::string response = "HTTP/1.1 301 Moved Permanently\r\nLocation: " + route->redirect.get() + "\r\nContent-Length: 0\r\n\r\n";
        send(_socket, response.c_str(), response.size(), 0);
    }

    switch (request.get_method()) {
        case GET:
            _handle_get_request(*route);
            break;
        case POST:
            _handle_post_request(*route);
            break;
        case DELETE:
            _handle_delete_request(*route);
            break;
        default:
            std::cerr << "Unknown method" << std::endl;
            break;
    }

    std::cout << "Processing request..." << std::endl;
}

bool Client::send_response() {
    // TODO obviously
	Response response;
	response.setStatusCode(301);
	response.setHeader("Content-Length", "0");
	response.setBody("");
    std::string response_str = response.toString();
    ssize_t bytes_sent = send(_socket, response_str.c_str(), response_str.length(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Failed to send response" << std::endl;
        return false;
    }
    std::cout << "Sent response: " << response_str << std::endl;
    return true;
}

bool Client::is_response_complete() const {
    return request.is_complete();
}

void Client::reset() {
    request.reset();
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
