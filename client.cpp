#include "Response.hpp"
#include "client.hpp"
#include "print.hpp"

#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <fstream>
#include <sstream>

std::string extractBoundary(const std::string& contentType) {
    size_t pos = contentType.find("boundary=");
    if (pos != std::string::npos) {
        size_t endPos = contentType.find(',', pos);
        if (endPos != std::string::npos) {
            return "--" + contentType.substr(pos + 9, endPos - pos - 9); // skip "boundary="
        } else
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

    DEBUG_IF(body.size(), body);
    DEBUG_IF_NOT(body.size(), "No body in request");
    DEBUG("Parsing multipart form data with boundary: " << boundary);

    while ((next = body.find(boundary, pos)) != std::string::npos) {
        DEBUG("Found boundary at position: " << next);
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

Client::Client(int socket, const ServerConfig& config) :
    _socket(socket), request(config.client_max_body_size.get()) {
    DEBUG("Client created with socket: " << socket);
}

Client::Client(const Client& other) :
    _socket(other._socket),
    request(other.request) {
    DEBUG("Client copy constructor called");
}

Client& Client::operator=(const Client& other) {
    if (this != &other) {
        _socket = other._socket;
        request = other.request;
        DEBUG("Client assignment operator called");
    }
    return *this;
}

Client::~Client() {
}

Client::Client() : _socket(-1), request(0) {
    DEBUG("Client default constructor called");
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
    char buffer[4096];
    ssize_t bytes_read = recv(_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read == 0)
        return false;

    while (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        request.append_data(std::string(buffer));
        bytes_read = recv(_socket, buffer, sizeof(buffer) - 1, 0);
    }
    if (bytes_read < 0) {
        perror("recv");
        return true;
    }

    return true;
}

bool Client::_try_create_response_from_file(const Path& filepath) {
    DEBUG("Trying to create response from file: " << filepath.get_path());
    std::ifstream file(filepath.get_path(), std::ios::binary);

    if (!file) {
        DEBUG("File not found");
        return (false);
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    file.close();

    response.setStatusCode(HttpStatusCode::OK);
    response.setHeader("Content-Type", get_mime_type(filepath.get_path()));
    response.setHeader("Content-Length", std::to_string(ss.str().length()));
    response.setBody(ss.str());
    DEBUG("Response created from file: " << filepath.get_path());
    return (true);
}

bool Client::_try_to_create_response_from_index(const Path& base_filepath, const IndexRule& index_rule) {
    for (const std::string &index : index_rule.get()) {
        Path index_path = base_filepath;
        index_path.append(index);
        DEBUG("Trying to create response from index file: " << index_path.get_path());
        if (_try_create_response_from_file(index_path)) {
            DEBUG("Response created from index file: " << index_path.get_path());
            return true;
        }
    }

    return false;
}

void Client::_handle_get_request(const LocationRule& route) {
    DEBUG("Handling GET request for route: " << route.get_path());
    if (!route.root.is_set()) {
        response.setStatusCode(HttpStatusCode::InternalServerError);
        ERROR("Root not set");
        return;
    }

    Path filepath = Path::create_from_url(request.get_path(), route);
    if (!filepath.is_valid()) {
        DEBUG("Invalid request path: " << filepath.get_path());
        response.setStatusCode(HttpStatusCode::BadRequest);
        return ;
    }

    struct stat path_stat{};
    if (stat(filepath.get_path().c_str(), &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
        DEBUG("Request path is a directory");
        if (route.index.is_set()) {
            if (!_try_to_create_response_from_index(filepath, route.index))
                response.setStatusCode(HttpStatusCode::NotFound);
            return ;
        } else {
            response.setStatusCode(HttpStatusCode::Forbidden);
            return ;
        }
    }

    DEBUG("Trying to fetch a file: " << filepath);
    if (!_try_create_response_from_file(filepath)) {
        DEBUG("File not found: " << filepath.get_path());
        response.setStatusCode(HttpStatusCode::NotFound);
        return ;
    }
}

void Client::_handle_post_request(const LocationRule& route) {
    DEBUG("Handling POST request for route: " << route.get_path());
    std::string content_type = request.get_header("Content-Type", "");
    DEBUG("Content-Type: " << content_type);

    if (!request.is_body_within_limits()) {
        response.setStatusCode(HttpStatusCode::PayloadTooLarge);
        DEBUG("Request body exceeds maximum size");
        return ;
    }

    if (content_type.find("multipart/form-data") == std::string::npos) {
        response.setStatusCode(HttpStatusCode::UnsupportedMediaType);
        DEBUG("Unsupported Content-Type: " << content_type);
        return ;
    }

    std::map<std::string, std::string> formFields;
    std::vector<FilePart> files;

    parseMultipartFormData(request.getBody(), extractBoundary(content_type), formFields, files);
    for (const FilePart& file : files) {
        Path uploadDir(route.upload_dir.get());
        uploadDir.append(file.filename);
        if (!uploadDir.is_valid()) {
            response.setStatusCode(HttpStatusCode::BadRequest);
            return ;
        }
        std::ofstream outFile(uploadDir.get_path(), std::ios::binary);
        if (outFile) {
            outFile.write(file.content.c_str(), file.content.length());
            outFile.close();
        } else {
            response.setStatusCode(HttpStatusCode::InternalServerError);
            return ;
        }
    }

    if (files.empty()) {
        response.setStatusCode(HttpStatusCode::BadRequest);
        response.setBody("No files uploaded");
        return ;
    }

    response.setStatusCode(HttpStatusCode::OK);
    response.setHeader("Content-Type", "text/plain");
    response.setBody("File(s) uploaded successfully");
}

void Client::_handle_delete_request(const LocationRule& route) {
    DEBUG("DELETE request received for path: " << request.get_path());
    std::map<std::string, std::string> queryParams = parseQueryParams(request.get_path());
    response.setHeader("Content-Type", "application/json");

    if (queryParams.find("file") == queryParams.end()) {
        response.setStatusCode(HttpStatusCode::BadRequest);
        response.setBody("{\"error\": \"Missing 'file' query parameter\"}");
        return ;
    }

    Path filepath(route.upload_dir.get());
    filepath.append(queryParams["file"]);
    if (!filepath.is_valid()) {
        response.setStatusCode(HttpStatusCode::BadRequest);
        response.setBody("{\"error\": \"Invalid file path\"}");
        return ;
    }

    if (std::remove(filepath.get_path().c_str()) != 0) {
        response.setStatusCode(HttpStatusCode::NotFound);
        response.setBody("{\"error\": \"File not found\"}");
        return ;
    }

    response.setStatusCode(HttpStatusCode::OK);
    response.setBody("{\"message\": \"File deleted successfully\"}");
}

void Client::process_request(const ServerConfig& config) {
    DEBUG("Processing request " << method_to_str(request.get_method()) << " for path: " << request.get_path());
    response = Response();

    const LocationRule *route = config.routes.find(request.get_path());
    if (route == nullptr) {
        response.setStatusCode(HttpStatusCode::NotFound);
        return ;
    }
    DEBUG("Route found: " << *route);

    if (!route->methods.is_allowed(request.get_method())) {
        response.setStatusCode(HttpStatusCode::MethodNotAllowed);
        return ;
    }

    if (route->redirect.is_set()) {
        response.setStatusCode(HttpStatusCode::MovedPermanently);
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
            ERROR("Unknown method: " << request.get_method());
            response.setStatusCode(HttpStatusCode::MethodNotAllowed);
            break;
    }
}

bool Client::send_response() {
    std::string response_str = response.toString();
    DEBUG("Sending response " << response.getStatusCode());
    ssize_t bytes_sent = send(_socket, response_str.c_str(), response_str.length(), 0);
    if (bytes_sent < 0) {
        ERROR("Failed to send response");
        return (false);
    }
    DEBUG("Response sent: " << response_str);
    DEBUG("Sent " << bytes_sent << " bytes to client");
    return (true);
}

bool Client::is_response_complete() const {
    return (true);
}

void Client::reset() {
    request.reset();
    DEBUG("Client reset");
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
