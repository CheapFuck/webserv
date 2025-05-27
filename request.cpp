#include "request.hpp"
#include "print.hpp"

#include <sys/socket.h>

Request::Request(const Request &other)
    : _buffer(other._buffer), metadata(other.metadata), headers(other.headers) {}

Request &Request::operator=(const Request &other) {
    if (this != &other) {
        _buffer = other._buffer;
        _contentLength = other._contentLength;
        _headersParsed = other._headersParsed;
        metadata = other.metadata;
        headers = other.headers;
    }
    return *this;
}

/// @brief Reads data from the socket into the request buffer.
/// @param fd The file descriptor of the socket to read from.
/// @return Returns false if the connection is closed, true otherwise.
bool Request::read(const int fd) {
    char buffer[READ_BUFFER_SIZE];
    ssize_t bytes_read;

    do {
        bytes_read = recv(fd, buffer, sizeof(buffer) - 1, 0);
        DEBUG("Received " << bytes_read << " bytes from fd: " << fd);

        if (bytes_read == 0) return (false);
        if (bytes_read < 0) return (true);

        buffer[bytes_read] = '\0';
        _buffer.append(buffer, bytes_read);
        _parse_request_headers();
    } while (bytes_read > 0);

    return (true);
}

/// @brief Extracts the most relevant configuration settings from the request headers
/// for reading the request body. Other headers are ignored; and up to the rest of
/// the program to handle.
bool Request::_fetch_config_from_headers() {
    try { _contentLength = std::stoul(headers.getHeader(HeaderKey::ContentLength, "")); }
    catch (...) {
        DEBUG("Invalid or missing Content-Length header value, defaulting to 0");
        _contentLength = 0;
    }

    return (true);
}

/// @brief Parses the request headers from the buffer.
/// @return Returns true if the headers are (already) parsed, false otherwise.
bool Request::_parse_request_headers() {
    if (_headersParsed)
        return (true);
    
    size_t header_data_end = _buffer.find("\r\n\r\n");
    if (header_data_end == std::string::npos)
        return (false);

    std::istringstream stream(_buffer.substr(0, header_data_end));
    metadata = RequestLine(stream);
    headers = Headers(stream);

    _buffer.erase(0, header_data_end + 4);
    _fetch_config_from_headers();
    _headersParsed = true;
    return (true);
}

/// @brief Checks if the request is ready to be processed.
/// @return Returns true if the request is ready, false otherwise.
bool Request::isComplete() const {
    if (!_headersParsed)
        return (false);
    
    DEBUG("Checking if request is complete for method: " << metadata.getMethod());
    DEBUG("Buffer length: " << _buffer.length() << ", Content-Length: " << _contentLength);
    
    switch (metadata.getMethod()) {
        case Method::GET:
        case Method::HEAD:
        case Method::DELETE:
            return (_buffer.empty());
        case Method::POST:
        case Method::PUT:
            return (_buffer.length() >= _contentLength);
        default:
            ERROR("Unknown method: " << metadata.getMethod());
            return (false);
    }
}
