#include "config/types/consts.hpp"
#include "config/rules/rules.hpp"
#include "methods.hpp"
#include "response.hpp"
#include "headers.hpp"
#include "print.hpp"

#include <sys/socket.h>
#include <sstream>
#include <ctime>

/// @brief Returns the current time as a formatted string in GMT.
static std::string get_time_as_readable_string() {
    std::time_t now = std::time(nullptr);
    char buffer[150];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&now));
    return std::string(buffer);
}

Response::Response() : _statusCode(HttpStatusCode::OK), _sentHeaders(false) {}

/// @brief Sets the status code for the response.
/// @param code The HTTP status code to set for the response.
void Response::setStatusCode(HttpStatusCode code) {
    _statusCode = code;
}

/// @brief Sets the status code for the response using a StatusCode enum.
void Response::setStatusCode(StatusCode code) {
    setStatusCode(fromStatusCode(code));
}

/// @brief Sets default headers for the response, including Content-Type and Date.
void Response::setDefaultHeaders() {
    headers.replace(HeaderKey::Date, get_time_as_readable_string());
}

bool Response::headersBeenSent() const {
    return (_sentHeaders);
}

bool Response::didResponseCreationFail() const {
    return (false);
}

void Response::sendHeaders(SocketFD &fd) {
    if (headersBeenSent())
        return ;

    _sentHeaders = true;
    std::ostringstream headerStream;
    headerStream << Response::protocol << "/" << Response::tlsVersion << " "
       << static_cast<int>(getStatusCode()) << " "
       << getStatusCodeAsStr(getStatusCode()) << "\r\n"
       << headers << "\r\n";

    DEBUG("Sending headers: " << headerStream.str());
    fd.writeAsString(headerStream.str());
}

/// @brief Returns the status code of the response.
HttpStatusCode Response::getStatusCode() const {
    return _statusCode;
}

HttpStatusCode Response::getFailedResponseStatusCode() const {
    return (HttpStatusCode::OK);
}

FileResponse::FileResponse(ReadableFD fileFD) :
    _fileFD(std::move(fileFD)) {
    headers.replace(HeaderKey::TransferEncoding, "chunked");
    DEBUG("FileResponse created with file FD: " << _fileFD.get());
}

bool FileResponse::isFullRequestBodyRecieved() const {
    return (true); // No body needed for FileResponse
}

bool FileResponse::isFullResponseSent() const {
    return (_fileFD.getReaderFDState() == FDState::Closed && headersBeenSent());
}

void FileResponse::handleRequestBody(SocketFD &fd) {
    fd.clearReadBuffer();
}

void FileResponse::handleSocketWriteTick(SocketFD &fd) {
    DEBUG("Handling socket write tick for FileResponse, fd: " << fd.get());
    if (!headersBeenSent()) {
        sendHeaders(fd);
        return ;
    }

    if (_fileFD.getReaderFDState() == FDState::Closed) {
        fd.writeAsChunk("");
        return ;
    }

    _fileFD.read();
    fd.writeAsChunk(_fileFD.extractFullBuffer());
}

void FileResponse::terminateResponse() {
    _fileFD.close();
}

FileResponse::~FileResponse() {
    DEBUG("FileResponse destroyed for file FD: " << _fileFD.get());
    _fileFD.close();
}






StaticResponse::StaticResponse(const std::string &content) :
    _content(content) {
    if (!_content.empty()) headers.replace(HeaderKey::ContentLength, std::to_string(_content.size()));
    DEBUG("StaticResponse created with content size: " << _content.size());
}

StaticResponse::~StaticResponse() {
    DEBUG("StaticResponse destroyed");
}

bool StaticResponse::isFullRequestBodyRecieved() const {
    return (true);
}

bool StaticResponse::isFullResponseSent() const {
    return (!headersBeenSent());
}

void StaticResponse::handleRequestBody(SocketFD &fd) {
    fd.clearReadBuffer();
}

void StaticResponse::handleSocketWriteTick(SocketFD &fd) {
    DEBUG("Handling socket write tick for StaticResponse, fd: " << fd.get());
    if (headersBeenSent()) {
        sendHeaders(fd);
        if (!_content.empty())
            fd.writeAsString(_content);
    }
}

void StaticResponse::terminateResponse() {
    DEBUG("Terminating StaticResponse");
}
