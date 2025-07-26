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

Response::Response() : _statusCode(HttpStatusCode::OK), _sentHeaders(false), _request(nullptr) {}

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

ssize_t Response::sendBodyAsChunk(SocketFD &fd, const std::string &body) {
	if (_request && _request->metadata.getMethod() == Method::HEAD) return static_cast<ssize_t>(body.length());
	return fd.writeAsChunk(body);
}

ssize_t Response::sendBodyAsString(SocketFD &fd, const std::string &body) {
	if (_request && _request->metadata.getMethod() == Method::HEAD) return static_cast<ssize_t>(body.length());
	return fd.writeAsString(body);
}

/// @brief Returns the status code of the response.
HttpStatusCode Response::getStatusCode() const {
    return _statusCode;
}

HttpStatusCode Response::getFailedResponseStatusCode() const {
    return (HttpStatusCode::OK);
}

FileResponse::FileResponse(ReadableFD fileFD, Request *request) :
    _fileFD(std::move(fileFD)), _isFinalChunkSent(false) {
	_request = request;
    headers.replace(HeaderKey::TransferEncoding, "chunked");
    DEBUG("FileResponse created with file FD: " << _fileFD.get());
}

bool FileResponse::isFullResponseSent() const {
    return (_isFinalChunkSent && headersBeenSent());
}

void FileResponse::handleRequestBody(SocketFD &fd, const Request &request) {
    switch (request.receivingBodyMode) {
        case ReceivingBodyMode::Chunked: {
            while (true) {
                FDReader::HTTPChunk chunk = fd.extractHTTPChunkFromReadBuffer();
                if (chunk.size == FDReader::HTTPChunk::noChunk) break ;
            }
            return ;
        }

        case ReceivingBodyMode::NotSet:
        case ReceivingBodyMode::ContentLength: {
            fd.clearReadBuffer();
            return ;
        }
    }
}

void FileResponse::handleSocketWriteTick(SocketFD &fd) {
    DEBUG("Handling socket write tick for FileResponse, fd: " << fd.get());
    if (!headersBeenSent()) {
        sendHeaders(fd);
        return ;
    }

    if (_fileFD.getReaderFDState() == FDState::Closed && _fileFD.getReadBufferSize() == 0 && !_isFinalChunkSent) {
        _isFinalChunkSent = true;
        sendBodyAsChunk(fd, "");
        return ;
    }

    _fileFD.read();
	std::string toSend = _fileFD.extractChunkFromReadBuffer(DEFAULT_CHUNK_SIZE);
	if (!toSend.empty())
    	sendBodyAsChunk(fd, toSend);
}

void FileResponse::terminateResponse() {
    _fileFD.close();
}

FileResponse::~FileResponse() {
    DEBUG("FileResponse destroyed for file FD: " << _fileFD.get());
    _fileFD.close();
}






StaticResponse::StaticResponse(const std::string &content, Request *request) :
    _content(content) {
	_request = request;
	if (_request->metadata.getMethod() != Method::HEAD)
    	headers.replace(HeaderKey::ContentLength, std::to_string(_content.size()));
	else headers.replace(HeaderKey::ContentLength, "0");
    DEBUG("StaticResponse created with content size: " << _content.size());
}

StaticResponse::~StaticResponse() {
    DEBUG("StaticResponse destroyed");
}

bool StaticResponse::isFullResponseSent() const {
    return (headersBeenSent());
}

void StaticResponse::handleRequestBody(SocketFD &fd, const Request &request) {
    switch (request.receivingBodyMode) {
        case ReceivingBodyMode::Chunked: {
            while (true) {
                FDReader::HTTPChunk chunk = fd.extractHTTPChunkFromReadBuffer();
                if (chunk.size == FDReader::HTTPChunk::noChunk) break ;
            }
            return ;
        }

        case ReceivingBodyMode::NotSet:
        case ReceivingBodyMode::ContentLength: {
            fd.clearReadBuffer();
            return ;
        }
    }
}

void StaticResponse::handleSocketWriteTick(SocketFD &fd) {
    DEBUG("Handling socket write tick for StaticResponse, fd: " << fd.get());
    if (!headersBeenSent())
        sendHeaders(fd);
    if (!_content.empty())
        sendBodyAsString(fd, _content);
}

void StaticResponse::terminateResponse() {
    DEBUG("Terminating StaticResponse");
}
