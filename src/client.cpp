#include "redact_dir_listing.cpp"
#include "client.hpp"
#include "methods.hpp"
#include "headers.hpp"
#include "server.hpp"
#include "print.hpp"
#include "fd.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <chrono>

Response *Client::_configureResponse(Response *response, HttpStatusCode statusCode) {
    response->setStatusCode(statusCode);
    response->setDefaultHeaders();

    if (request.headers.getHeader(HeaderKey::Connection, "keep-alive") == "close")
        response->headers.replace(HeaderKey::Connection, "close");

    _server.fetchUserSession(request, *response);

    return (response);
}

static const std::string getDefaultBody(HttpStatusCode statusCode) {
    std::string todisplay = std::to_string(static_cast<int>(statusCode)) + " - " + getStatusCodeAsStr(statusCode);
    std::string s = R"(<!DOCTYPE html>
        <html lang="en">
        <head>
        <meta charset="UTF-8">
        <title>[TITLEPLACE]</title>
        <style>
            html, body {
            height: 100%;
            margin: 0px;
            }
            body {
            display: flex;
            justify-content: center;
            border-style: inset;
            border-width: 9px;
            align-items: center;
            font-size: 2em;
            background-color: #F0F0F0
        ;
            }
        </style>
        </head>
        <body>
        Error [ERRORTEXTPLACE]
        </body>
        </html>)";
    s.replace(s.find("[ERRORTEXTPLACE]"), 16, todisplay);
    s.replace(s.find("[TITLEPLACE]"), 12, todisplay);

    return (s);
}

Response *Client::_createErrorResponse(HttpStatusCode statusCode, const LocationRule &route, bool shouldCloseConnection) {
    DEBUG("Creating error response");
    std::string errorPage = route.errorPages.getErrorPage(StatusCode(static_cast<int>(statusCode)));
    Response *ret = nullptr;

    if (!errorPage.empty()) {
        int fd = open(errorPage.c_str(), O_RDONLY);
        if (fd != -1)
            ret = _configureResponse(new FileResponse(this, ReadableFD::file(fd), &request), statusCode);
    }

    if (!ret)
        ret = _configureResponse(new StaticResponse(this, getDefaultBody(statusCode), &request), statusCode);

    if (shouldCloseConnection && ret)
        ret->headers.replace(HeaderKey::Connection, "close");

    return (ret);
}

Response *Client::_createReturnRuleResponse(const ReturnRule &returnRule) {
    DEBUG("Creating return rule response for: " << returnRule.getParameter());
    Response *response = _configureResponse(new StaticResponse(this, returnRule.getParameter(), &request), static_cast<HttpStatusCode>(int(returnRule.getStatusCode())));
    if (returnRule.isRedirect())
        response->headers.replace(HeaderKey::Location, returnRule.getParameter());
    return (response);
}

Response *Client::_createDirectoryListingResponse(const LocationRule &route) {
    DEBUG("Creating directory listing response for route: " << route);
    if (!route.autoIndex.get())
        return (_createErrorResponse(HttpStatusCode::NotFound, route));

    Response *response = _configureResponse(new StaticResponse(this, redactDirectoryListing(request.metadata.getPath().str(), request.metadata.getRawUrl()), &request), HttpStatusCode::OK);
    response->headers.replace(HeaderKey::ContentType, "text/html");
    return (response);
}

Response *Client::_createCGIResponse(SocketFD &fd, const ServerConfig &config, const LocationRule &route) {
    DEBUG("Creating CGI response for route: " << route);
    CGIResponse *response = new CGIResponse(this, _server, fd, &request);
    _configureResponse(response, HttpStatusCode::OK);
    if (!response->start(config, route, Path(_server.getServerExecutablePath()))) {
        delete response;
        return _createErrorResponse(HttpStatusCode::InternalServerError, route);
    }
    return (response);
}

Client::Client(Server &server, int serverFd, const char *clientIP, int clientPort) :
    _server(server),
    _serverFd(serverFd),
    _state(ClientHTTPState::WaitingForHeaders),
    _chunkedRequestBodyRead(false),
    _clientIP(std::string(clientIP)),
    _clientPort(std::to_string(clientPort)),
    response(nullptr),
    request() {}

bool Client::isFullRequestBodyReceived(SocketFD &fd) const {
    switch (request.receivingBodyMode) {
        case ReceivingBodyMode::Chunked: {
            return (fd.isFinalChunkRead());
        }
        case ReceivingBodyMode::ContentLength:
        default: {
            return (fd.getTotalBodyBytes() - request.headerPartLength >= request.contentLength);
        }
    }
}

Response *Client::_createResponseFromRequest(SocketFD &fd, Request &request) {
    DEBUG("Creating response from request for Client, fd: " << fd.get());

    ServerConfig &config = _server.loadRequestConfig(request, _serverFd);
    route = &config.getLocation(request.metadata.getRawUrl());
    request.metadata.translateUrl(_server.getServerExecutablePath(), *route);

    DEBUG("Route found for request: " << *route);
    DEBUG("URL path: " << request.metadata.getPath().str());

    if ((route->maxBodySize.isSet() && request.contentLength > route->maxBodySize.getMaxBodySize().get()))
        return _createErrorResponse(HttpStatusCode::PayloadTooLarge, *route);

    if (route->cgi.isEnabled() || (!request.metadata.pathIsDirectory() && route->cgiExtension.isCGI(request.metadata.getPath())))
        return _createCGIResponse(fd, config, *route);

    if (!route->methods.isAllowed(request.metadata.getMethod()))
        return _createErrorResponse(HttpStatusCode::MethodNotAllowed, *route);

    if (route->returnRule.isSet())
        return _createReturnRuleResponse(route->returnRule);

    if (!(route->root.isSet() || route->alias.isSet()))
        return _createErrorResponse(HttpStatusCode::NotFound, *route);

    if (!request.metadata.getPath().isValid())
        return _createErrorResponse(HttpStatusCode::BadRequest, *route);

    if (request.metadata.pathIsDirectory())
        return _createDirectoryListingResponse(*route);

    if (request.metadata.getMethod() != Method::GET)
        return _createErrorResponse(HttpStatusCode::BadRequest, *route);

    int fileFd = open(request.metadata.getPath().str().c_str(), O_RDONLY);
    if (fileFd == -1)
        return _createErrorResponse(HttpStatusCode::NotFound, *route);

    if (request.metadata.getRawUrl() == "/directory") {
        return _configureResponse(new StaticResponse(this, "", &request), HttpStatusCode::OK);
    }

    return (_configureResponse(new FileResponse(this, ReadableFD::file(fileFd), &request), HttpStatusCode::OK));
}

bool Client::setEpollWriteNotification(SocketFD &fd) {
    DEBUG("Setting EPOLLOUT for Client: " << fd.get());
    if (fd.setEpollEvents(EPOLLOUT | EPOLLIN) == -1) {
        ERROR("Failed to set EPOLLOUT for client: " << fd.get());
        _server.untrackClient(fd);
        return (false);
    }
    return (true);
}

bool Client::unsetEpollWriteNotification(SocketFD &fd) {
    DEBUG("Unsetting EPOLLOUT for Client: " << fd.get());
    if (fd.setEpollEvents(EPOLLIN) == -1) {
        ERROR("Failed to unset EPOLLOUT for client: " << fd.get());
        _server.untrackClient(fd);
        return (false);
    }
    return (true);
}

void Client::handleRead(SocketFD &fd, ssize_t funcReturnValue) {
    DEBUG("Handling read for Client, fd: " << fd.get() << ", state: " << static_cast<int>(_state));
    switch (_state) {
        case ClientHTTPState::Idle: {
            if (fd.getReadBufferSize() > 0) {
                _state = ClientHTTPState::WaitingForHeaders;
                return handleRead(fd, funcReturnValue);
            }
            return ;
        }

        case ClientHTTPState::WaitingForHeaders: {
            std::string headerString = fd.extractHeadersFromReadBuffer();
            if (headerString.empty()) {
                if (fd.wouldReadExceedMaxBufferSize()) {
                    DEBUG("No valid headers received; closing the connection.");
                    _server.untrackClient(fd);
                }
                return ;
            }

            request = Request(headerString);
            response = _createResponseFromRequest(fd, request);

            if (response->shouldDirectlySendResponse() &&
                !setEpollWriteNotification(fd)) return ;

            _state = ClientHTTPState::ReadingBody;
            return handleRead(fd, funcReturnValue);
        }

        case ClientHTTPState::ReadingBody: {
            response->handleRequestBody(fd, request);

            if (request.receivingBodyMode == ReceivingBodyMode::Chunked) {
                FDReader::HTTPChunkStatus chunkStatus = fd.returnHTTPChunkStatus();
                if (chunkStatus == FDReader::HTTPChunkStatus::Error) {
                    DEBUG("Error reading chunked body, using error response");
                    switchResponseToErrorResponse(HttpStatusCode::BadRequest, fd);
                    return ;
                } else if (chunkStatus == FDReader::HTTPChunkStatus::TooLarge) {
                    DEBUG("Chunk size exceeds maximum allowed size, using error response");
                    switchResponseToErrorResponse(HttpStatusCode::PayloadTooLarge, fd);
                    return ;
                }
            }

            if (route && route->maxBodySize.isSet() && route->maxBodySize.getMaxBodySize().get() < static_cast<size_t>(fd.getTotalBodyBytes())) {
                DEBUG("Request body exceeds max body size, using error response");
                switchResponseToErrorResponse(HttpStatusCode::PayloadTooLarge, fd);
                return ;
            }

            if (isFullRequestBodyReceived(fd))
                _state = ClientHTTPState::SendingResponse;

            return ;
        }

        default: {
            ERROR("Unexpected state in Client::handleRead: " << static_cast<int>(_state));
            return ;
        }
    }
}

void Client::handleWrite(SocketFD &fd) {
    DEBUG("Handling write for Client, fd: " << fd.get());

    if (!response) {
        ERROR("No response to write for Client: " << fd.get());
        return;
    }

    if (isFullRequestBodyReceived(fd))
        _state = ClientHTTPState::SendingResponse;

    response->handleSocketWriteTick(fd);

    if (response->isFullResponseSent())
        handleClientReset(fd);
}

void Client::handleClientReset(SocketFD &fd) {
    DEBUG("Full response sent for Client, fd: " << fd.get());

	DEBUG("Socket body bytes" << fd.getTotalBodyBytes());
	DEBUG("Socket wrote bytes" << response->fuckyou());
    if (response) {
        delete response;
        response = nullptr;
    }

    if (request.headers.getHeader(HeaderKey::Connection, "keep-alive") == "close") {
        DEBUG("Connection header indicates 'close', disconnecting Client: " << fd.get());
        _server.untrackClient(fd);
        return;
    }

    if (fd.setEpollEvents(EPOLLIN) == -1) {
        ERROR("Failed to set EPOLLIN for client: " << fd.get());
        _server.untrackClient(fd);
        return;
    }

    _state = ClientHTTPState::Idle;
    request = Request();
    fd.resetCounter();

    _chunkedRequestBodyRead = false;
}

void Client::switchResponseToErrorResponse(HttpStatusCode statusCode, SocketFD &fd) {
    DEBUG("SWITCHING response to error response for Client: " << _clientIP << ":" << _clientPort);
    DEBUG("Current response: " << (response ? "exists" : "does not exist"));
    DEBUG("Response address: " << response);
    if (response) {
        DEBUG("Deleting existing response for Client: " << _clientIP << ":" << _clientPort);
        delete response;
    }

    ServerConfig &config = _server.loadRequestConfig(request, _serverFd);
    response = _createErrorResponse(statusCode, config.getLocation(request.metadata.getRawUrl()), true);

    if (_state == ClientHTTPState::WaitingForHeaders || _state == ClientHTTPState::ReadingBody)
        _state = ClientHTTPState::SendingResponse;

    if (!setEpollWriteNotification(fd))
        return ;
}

bool Client::isTimedOut(const HTTPRule &httpRule, const SocketFD &fd) const {
    switch (getState()) {
        case ClientHTTPState::WaitingForHeaders: {
            auto bound = fd.getLastReadTime() + std::chrono::duration<double>(httpRule.clientHeaderTimeout.timeout.getSeconds());
            DEBUG_IF(bound < std::chrono::steady_clock::now(), "Client timed out while waiting for headers, fd: " << fd.get() << ", bound: " << bound.time_since_epoch().count());
            return (bound < std::chrono::steady_clock::now());
        }

        case ClientHTTPState::ReadingBody: {
            ServerConfig &config = _server.loadRequestConfig(request, _serverFd);
            const LocationRule &route = config.getLocation(request.metadata.getRawUrl());

            auto bound = fd.getLastReadTime() + std::chrono::duration<double>(route.clientBodyReadTimeout.timeout.getSeconds());
            DEBUG_IF(bound < std::chrono::steady_clock::now(), "Client timed out while reading body, fd: " << fd.get() << ", bound: " << bound.time_since_epoch().count());
            return (bound < std::chrono::steady_clock::now());
        }

        default: {
            return (false);
        }
    }
}

bool Client::shouldBeClosed(const HTTPRule &httpRule, const SocketFD &fd) const {
    if (getState() == ClientHTTPState::Idle) {
        auto bound = fd.getLastReadTime() + std::chrono::duration<double>(httpRule.clientKeepAliveReadTimeout.timeout.getSeconds());
        DEBUG_IF(bound < std::chrono::steady_clock::now(), "Client timed out while waiting for next request, fd: " << fd.get() << ", bound: " << bound.time_since_epoch().count());
        return (bound < std::chrono::steady_clock::now());
    }

    return (false);
}

Client::~Client() {
    DEBUG("Destroying client for IP: " << _clientIP << ", Port: " << _clientPort << " (" << this << ")");
    if (response)
        delete response;
}

ClientHTTPState Client::getState() const {
    return (_state);
}
