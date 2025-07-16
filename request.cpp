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
	std::cout << "Headers are: '''" << headers << "'''\n";
	if (headers.isValid())
    	_headersParsed = true;
    return (_headersParsed);
}

bool Request::parseRequestBody(std::string &buffer)
{
	int length_cur = 0;
	if (!_headersParsed)
		return (false);
	if (_request_complete)
	{
		ERROR("Trying to parse an already complete body for a request, body was " << _body);
		return (true);
	}
	const std::string encoding = headers.getHeader(HeaderKey::TransferEncoding, "");
	if (encoding == "chunked")
	{
		if (buffer.find("\r\n") != std::string::npos)
		{
			std::cout << "Stoiing : " << buffer << "!!!\n";
			length_cur = std::stoi(buffer);
			if (buffer.length() < buffer.find("\r\n") + 4 + length_cur) return (false);
			if (!length_cur)
				_request_complete = true;
			else {
				_body.append(buffer.substr(buffer.find("\r\n") + 2, length_cur));
				buffer.erase(0, buffer.find("\r\n") + 4 + length_cur);
			}
		}
		std::cout << "Chunkening : '''" << _body << "'''\n";
	}
	else if (encoding != "")
	{
		setBody(buffer);
		if (_body.length() >= _contentLength)
			_request_complete = true;
	}
	return (_request_complete);
}

/// @brief Checks if the request is ready to be processed.
/// @return Returns true if the request is ready, false otherwise.
bool Request::isComplete(const std::string &buffer) const {
	(void)buffer;
    if (!_headersParsed)
        return (false);

    switch (metadata.getMethod()) {
        case Method::POST:
        case Method::GET:
        case Method::HEAD:
        case Method::DELETE:
		// return (buffer.empty());
        case Method::PUT:
		//return (buffer.length() >= _contentLength);
		return (_request_complete);
        default:
            ERROR("Unknown method: " << metadata.getMethod());
            return (false);
    }
}
