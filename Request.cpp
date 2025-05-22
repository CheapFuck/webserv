#include "Request.hpp"
#include <iostream>
#include <sstream>
#include "config/consts.hpp"

Request::Request() 
{

}

Request::~Request() 
{

}

bool Request::is_headers_received() {
    if (_buffer.find("\r\n\r\n") == std::string::npos) {
        return (false);
    }
    parse_headers();
    return (true);
}

bool Request::parse_headers() 
{
    std::istringstream stream(_buffer);
    std::string line;

    std::cout << _buffer << std::endl;
    while (std::getline(stream, line, '\n')) 
    {
        // Remove trailing \r
        //if (!line.empty() && line.back() == '\r') line.pop_back();

        // Find position of colon
        size_t colon = line.find(':');
        if (colon != std::string::npos) 
        {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            _headers_dict[key] = value;
            //std::cout << "Header " << key << get_header(key) << std::endl;
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

