#include "request.hpp"
#include "print.hpp"

#include <sys/socket.h>

Request::Request(const Request &other)
    : _contentLength(other._contentLength), _cookies(other._cookies), _body(other._body), _headersParsed(other._headersParsed), metadata(other.metadata), headers(other.headers), session(other.session) {}

Request &Request::operator=(const Request &other) {
    if (this != &other) {
        _contentLength = other._contentLength;
        _cookies = other._cookies;
        _body = other._body;
        _headersParsed = other._headersParsed;
        metadata = other.metadata;
        headers = other.headers;
        session = other.session;
    }
    return *this;
}


int hexCharToInt(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    return -1; // invalid hex char
}

static std::string urlDecode(const std::string& str) {
    std::string result;
    for (std::size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int high = hexCharToInt(str[i + 1]);
            int low = hexCharToInt(str[i + 2]);
            if (high != -1 && low != -1) {
                char decodedChar = static_cast<char>((high << 4) | low);
                result += decodedChar;
                i += 2;
            } else {
                // Invalid encoding, keep the '%'
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
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

    _cookies = Cookie::createAllFromHeader(headers.getHeader(HeaderKey::Cookie, ""));
    return (true);
}

/// @brief Parses the request headers from the buffer.
/// @return Returns true if the headers are (already) parsed, false otherwise.
bool Request::parseRequestHeaders(std::string &buffer) {
    if (_headersParsed)
        return (true);

    size_t header_data_end = buffer.find("\r\n\r\n");
    if (header_data_end == std::string::npos)
        return (false);

    std::istringstream stream(buffer.substr(0, header_data_end));
    metadata = RequestLine(stream);
    headers = Headers(stream);

    buffer.erase(0, header_data_end + 4);
    _fetch_config_from_headers();
    _headersParsed = true;
    return (true);
}

/// @brief Checks if the request is ready to be processed.
/// @return Returns true if the request is ready, false otherwise.
bool Request::isComplete(const std::string &buffer) const {
    if (!_headersParsed)
        return (false);

    switch (metadata.getMethod()) {
        case Method::GET:
        case Method::HEAD:
        case Method::DELETE:
            // return (buffer.empty());
        case Method::POST:
        case Method::PUT:
            return (buffer.length() >= _contentLength);
        default:
            ERROR("Unknown method: " << metadata.getMethod());
            return (false);
    }
}
