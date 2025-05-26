#include "print.hpp"
#include "Utils.hpp"
#include "Request.hpp"
#include "config/consts.hpp"
#include "config/rules/rules.hpp"

#include <iostream>
#include <sstream>
#include <string.h>

size_t get_content_length(const std::string &value) {
    size_t length = 0;
    try {
        length = std::stoul(value);
    } catch (const std::invalid_argument&) {
        ERROR("Invalid Content-Length value: " << value);
        return 0;
    } catch (const std::out_of_range&) {
        ERROR("Content-Length value out of range: " << value);
        return 0;
    }
    return length;
}

Request::Request(size_t max_body_size) {
    _max_body_size = max_body_size;
    _headers_parsed = false;
}

Request::~Request() {}

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
        _max_body_size = other._max_body_size;
        _headers_dict = other._headers_dict;
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
            _content_length = get_content_length(get_header("Content-Length", "0"));
            DEBUG("Headers parsed, Content-Length: " << _content_length);
        }
    }
}

bool Request::is_headers_received() {
    return (_headers_parsed || _buffer.find("\r\n\r\n") != std::string::npos);
}

bool Request::is_body_within_limits() const {
    if (!_headers_parsed) return true;
    return (_buffer.size() <= _max_body_size && _content_length <= _max_body_size);
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

    DEBUG_IF(_method == UNKNOWN_METHOD, "Unknown method: " << method_str);

    if (_method == UNKNOWN_METHOD) {
        ERROR("Unknown method: " << method_str);
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
            if (_headers_dict.find(key) != _headers_dict.end()) 
                _headers_dict[key] += ", " + value;
            else
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

const std::string& Request::get_header(const std::string& key, const std::string& default_value) const {
    std::unordered_map<std::string, std::string>::const_iterator iter = _headers_dict.find(key);
    if (iter != _headers_dict.end())
        return (iter->second);
    return (default_value);
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

const std::unordered_map<std::string, std::string>& Request::get_headers() const {
    return (_headers_dict);
}

bool Request::is_complete() const {
    // Headers not parsed yet; should be considered incomplete
    if (!_headers_parsed) return (false);

    switch (_method) {
        case GET:
        case HEAD:
        case DELETE:
            return (_buffer.empty());
        case POST:
        case PUT:
            return (_buffer.length() >= _content_length);
        default:
            ERROR("Unknown method: " << _method);
            return (false);
            break;
    }
}

void Request::reset() {
    _buffer.clear();
    _method = UNKNOWN_METHOD;
    _path.clear();
    _version.clear();
    _headers_dict.clear();
    _headers_parsed = false;
    _content_length = 0;
}

const std::string& Request::getBody() const {
    if (_headers_parsed) return (_buffer);
    throw std::runtime_error("Headers not parsed yet");
}
