#include "request.hpp"
#include "print.hpp"
#include "fd.hpp"

#include <sys/socket.h>

/// @brief Extracts the most relevant configuration settings from the request headers
/// for reading the request body. Other headers are ignored; and up to the rest of
/// the program to handle.
void Request::_fetch_config_from_headers() {
    if (headers.getHeader(HeaderKey::TransferEncoding, "") == "chunked")
        receivingBodyMode = ReceivingBodyMode::Chunked;
    else if (headers.getHeader(HeaderKey::ContentLength, "") != "") {
        receivingBodyMode = ReceivingBodyMode::ContentLength;

        try { contentLength = std::stoul(headers.getHeader(HeaderKey::ContentLength, "")); }
        catch (...) {
            DEBUG_IF(receivingBodyMode == ReceivingBodyMode::ContentLength, "Invalid Content-Length header value, defaulting to 0");
            contentLength = 0;
        }
    } else {
        DEBUG("Neither Transfer-Encoding nor Content-Length headers are set, defaulting to NotSet");
        receivingBodyMode = ReceivingBodyMode::NotSet;
        contentLength = 0;
    }

    cookies = Cookie::createAllFromHeader(headers.getHeader(HeaderKey::Cookie, ""));
}

/// @brief Parses the request headers from the buffer.
/// @return Returns true if the headers are (already) parsed, false otherwise.
Request::Request(std::string &buffer) :
    metadata(), headers(), contentLength(0), headerPartLength(0), cookies(), session(nullptr), receivingBodyMode(ReceivingBodyMode::NotSet)
{
    std::istringstream stream(buffer);
    metadata = RequestLine(stream);
    headers = Headers(stream);

    _fetch_config_from_headers();
	DEBUG("Headers are: '''" << headers << "'''\n");
}

