#include "client.hpp"
#include "Response.hpp"

#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <fstream>
#include <sstream>

std::string extractBoundary(const std::string& contentType) {
    size_t pos = contentType.find("boundary=");
    if (pos != std::string::npos) {
        return "--" + contentType.substr(pos + 9); // skip "boundary="
    }
    return "";
}

std::string extractBetween(const std::string& str, const std::string& start, const std::string& end) {
    size_t s = str.find(start);
    if (s == std::string::npos) return "";
    s += start.length();
    size_t e = str.find(end, s);
    return (e == std::string::npos) ? "" : str.substr(s, e - s);
}

void parseMultipartFormData(const std::string& body, const std::string& boundary,
    std::map<std::string, std::string>& formFields,
    std::vector<FilePart>& files) {
    size_t pos = 0, next;
    std::cout << body << std::endl;
    while ((next = body.find(boundary, pos)) != std::string::npos) {
        size_t headerEnd = body.find("\r\n\r\n", pos);
        if (headerEnd == std::string::npos) break;

        std::string headers = body.substr(pos, headerEnd - pos);
        size_t contentStart = headerEnd + 4;
        size_t contentEnd = body.find(boundary, contentStart) - 2; // exclude \r\n

        std::string content = body.substr(contentStart, contentEnd - contentStart);
        FilePart part;

        if (headers.find("filename=\"") != std::string::npos) {
            part.name = extractBetween(headers, "name=\"", "\"");
            part.filename = extractBetween(headers, "filename=\"", "\"");
            part.content = content;
            files.push_back(part);
        } else {
            std::string name = extractBetween(headers, "name=\"", "\"");
            formFields[name] = content;
        }

        pos = contentEnd + boundary.length() + 2; // move to next part
    }
}

std::map<std::string, std::string> parseQueryParams(const std::string& url) {
    std::map<std::string, std::string> params;
    size_t queryStart = url.find("?");
    
    if (queryStart == std::string::npos) {
        return params;  // No query string, return empty map
    }

    std::string queryString = url.substr(queryStart + 1);  // Skip past "?"
    std::istringstream queryStream(queryString);
    std::string param;

    while (std::getline(queryStream, param, '&')) {
        size_t eqPos = param.find("=");
        if (eqPos != std::string::npos) {
            std::string key = param.substr(0, eqPos);
            std::string value = param.substr(eqPos + 1);
            params[key] = value;
        }
    }

    return params;
}

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

Client::~Client() {
}

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
        response.setStatusCode(500);
        std::cerr << "Root not set" << std::endl;
        return;
    }

    std::string rootDir = route.root.get().get_path() + "/";

    std::cout << request.get_path() << std::endl;
    std::cout << rootDir << std::endl;

    std::string request_path = std::string(request.get_path()).replace(0, route.get_path().length(), rootDir);

    if (request_path.find("..") != std::string::npos) {
        response.setStatusCode(400);
        return ;
    }

    struct stat path_stat{};
    if (stat(request_path.c_str(), &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
        std::cout << "Request path is a directory" << std::endl;
        if (route.index.is_set()) {
            request_path = request_path + "/" + route.index.get();
            std::cout << "Request path: " << request_path << std::endl;
        } else {
            response.setStatusCode(403);
            return ;
        }
    }

    std::cout << "Request path: " << request_path << std::endl;
    std::ifstream file(request_path, std::ios::binary);
    if (!file) {
        response.setStatusCode(404);
        return ;
    } else {
        std::ostringstream ss;
        ss << file.rdbuf();
        file.close();

        response.setStatusCode(200);
        response.setHeader("Content-Type", get_mime_type(request_path));
        response.setHeader("Content-Length", std::to_string(ss.str().length()));
        response.setHeader("Connection", "close");
        response.setBody(ss.str());
    } 
}

void Client::_handle_post_request(const LocationRule& route) {
    std::cout << "Handling POST request" << std::endl;
    std::string content_type = request.get_header("Content-Type", "");
    if (content_type.find("multipart/form-data") == std::string::npos) {
        response.setStatusCode(415);
        return ;
    }

    std::map<std::string, std::string> formFields;
    std::vector<FilePart> files;

    parseMultipartFormData(request.getBody(), extractBoundary(content_type), formFields, files);
    for (const FilePart& file : files) {
        std::ofstream outFile((route.upload_dir.get().get_path() + "/" + file.filename).c_str(), std::ios::binary);
        if (outFile) {
            outFile.write(file.content.c_str(), file.content.length());
            outFile.close();
        } else {
            response.setStatusCode(500);
            return ;
        }
    }

    response.setStatusCode(200);
    response.setHeader("Content-Type", "text/plain");
    response.setBody("File(s) uploaded successfully");
}

void Client::_handle_delete_request(const LocationRule& route) {
    (void)route;
    std::cout << "Handling DELETE request" << std::endl;
    std::map<std::string, std::string> queryParams = parseQueryParams(request.get_path());

    if (queryParams.find("file") == queryParams.end()) {
        response.setStatusCode(400);
        return ;
    }

    std::string filePath = queryParams["file"];

    if (filePath.find("..") != std::string::npos || filePath.find("/uploads/") == std::string::npos) {
        response.setStatusCode(400);
        return ;
    }

    std::string fullPath = "var/www/uploads/" + filePath;
    if (std::remove(fullPath.c_str()) != 0) {
        response.setStatusCode(404);
        return ;
    }

    response.setStatusCode(200);
}

void Client::process_request(const ServerConfig& config) {
    response = Response();

    const LocationRule *route = config.routes.find(request.get_path());
    std::cout << "Route: " << route << std::endl;
    if (route == nullptr) {
        response.setStatusCode(404);
        return ;
    }
    std::cout << "Route found: " << *route << std::endl;

    if (!route->methods.is_allowed(request.get_method())) {
        response.setStatusCode(405);
        return ;
    }

    if (route->redirect.is_set()) {
        response.setStatusCode(301);
        response.setHeader("Location", route->redirect.get());
        return ;
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
    std::string response_str = response.toString();
	std::cout << "Sending "<< response_str;
    ssize_t bytes_sent = send(_socket, response_str.c_str(), response_str.length(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Failed to send response" << std::endl;
        return false;
    }
    std::cout << "Sent response: " << response_str << std::endl;
    return true;
}

bool Client::is_response_complete() const {
    return true;
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
