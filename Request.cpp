#include "Request.hpp"
#include <iostream>
#include <sstream>
#include <string.h>
#include "Utils.hpp"
#include "config/consts.hpp"

Request::Request() 
{

}

Request::~Request() 
{

}

Request::Request(const Request& other) 
{
    *this = other;
}

Request& Request::operator=(const Request& other) 
{
    if (this != &other) {
        _buffer = other._buffer;
        _method = other._method;
        _path = other._path;
        _version = other._version;
        _headers_dict = other._headers_dict;
        _body = other._body;
        _headers_parsed = other._headers_parsed;
        _content_length = other._content_length;
    }
    return *this;
}

void Request::append_data(const std::string& data) {
    _buffer.append(data);

    if (is_headers_received()) {
        if (_headers_dict.empty()) {
            parse_headers();
            _headers_parsed = true;
            _buffer.erase(0, _buffer.find("\r\n\r\n") + 4);
        }
    }
}

bool Request::is_headers_received() {
    return (_buffer.find("\r\n\r\n") != std::string::npos);
}

bool Request::_parse_request_line(const std::string& line) 
{
    std::istringstream stream(line);
    std::string method_str;
    std::string path;
    std::string version;

    stream >> method_str >> path >> version;

    if (path.empty() || version.empty()) {
        return (false);
    }

    _method = string_to_method(method_str);
    _path = path;
    _version = version;

    if (_method == UNKNOWN_METHOD) {
        std::cerr << "Unknown method: " << method_str << std::endl;
        return (false);
    }

    return (true);
}

bool Request::parse_headers() 
{
    std::istringstream stream(_buffer);
    std::string line;
    bool parsed_first_line = false;

    while (std::getline(stream, line, '\n')) 
    {
        line = Utils::trim(line);
        if (!parsed_first_line) {
            if (!_parse_request_line(line))
                return (false);
            parsed_first_line = true;
            continue;
        }

        // Find position of colon
        size_t colon = line.find(':');
        if (colon != std::string::npos) 
        {
            std::string key = Utils::trim(line.substr(0, colon));
            std::string value = Utils::trim(line.substr(colon + 1));
            _headers_dict[key] = value;
        }   
    }

    return (true);
}

const std::string& Request::get_header(const std::string& key) const
{
    static const std::string empty;

    std::unordered_map<std::string, std::string>::const_iterator iter = _headers_dict.find(key);
    if (iter != _headers_dict.end())
        return (iter->second);
    throw ; // Remove me 
    return (empty);
}

const std::string& Request::get_path() const {
    return (_path);
}

const std::string& Request::get_version() const {
    return (_version);
}

const Method& Request::get_method() const {
    return (_method);
}

bool Request::is_complete() const {
    if (!_headers_parsed) return (false);
    switch (_method) {
        case GET:
        case HEAD:
            return (_buffer.empty());
        case POST:
        case PUT:
            return (_buffer.length() >= _content_length);
        default:
            std::cerr << "Unknown method: " << _method << std::endl;
            return (false);
            break;
    }
}

void Request::reset() {
    _buffer.clear();
    _headers_dict.clear();
    _body.clear();
    _method = UNKNOWN_METHOD;
    _path.clear();
    _version.clear();
    _headers_parsed = false;
    _content_length = 0;
}
