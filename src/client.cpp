#include "redact_dir_listing.cpp"
#include "client.hpp"
#include "methods.hpp"
#include "headers.hpp"
#include "server.hpp"
#include "print.hpp"
#include "fd.hpp"

#include <unistd.h>
#include <fcntl.h>

Response *Client::_configureResponse(Response *response, HttpStatusCode statusCode) {
    response->setStatusCode(statusCode);
    response->setDefaultHeaders();

    if (request.headers.getHeader(HeaderKey::Connection, "keep-alive") == "close")
        response->headers.replace(HeaderKey::Connection, "close");

    _server.fetchUserSession(request, *response);

    return (response);
}

Response *Client::_createErrorResponse(HttpStatusCode statusCode, const LocationRule &route) {
    std::string errorPage = route.errorPages.getErrorPage(StatusCode(static_cast<int>(statusCode)));
    if (!errorPage.empty()) {
        int fd = open(errorPage.c_str(), O_RDONLY);
        if (fd != -1)
            return (_configureResponse(new FileResponse(ReadableFD::file(fd)), statusCode));
    }

    return (_configureResponse(new StaticResponse(getStatusCodeAsStr(statusCode)), statusCode));
}

Response *Client::_createReturnRuleResponse(const ReturnRule &returnRule) {
    Response *response = _configureResponse(new StaticResponse(returnRule.getParameter()), static_cast<HttpStatusCode>(int(returnRule.getStatusCode())));
    if (returnRule.isRedirect()) 
        response->headers.replace(HeaderKey::Location, returnRule.getParameter());
    return (response);
}

Response *Client::_createDirectoryListingResponse(const LocationRule &route) {
    if (!route.autoIndex.get())
        return (_createErrorResponse(HttpStatusCode::Forbidden, route));

    Response *response = _configureResponse(new StaticResponse(redactDirectoryListing(request.metadata.getPath().str(), request.metadata.getRawUrl())), HttpStatusCode::OK);
    response->headers.replace(HeaderKey::ContentType, "text/html");
    return (response);
}

Response *Client::_createCGIResponse(SocketFD &fd, const ServerConfig &config, const LocationRule &route) {
    CGIResponse *response = new CGIResponse(_server, fd, shared_from_this());
    _configureResponse(response, HttpStatusCode::OK);
    response->start(config, route);
    if (response->didResponseCreationFail()) {
        delete response;
        return _createErrorResponse(response->getFailedResponseStatusCode(), route);
    }
    return (response);
}

Client::Client(Server &server, int serverFd, const char *clientIP, int clientPort) :
    _server(server),
    _serverFd(serverFd),
    _state(ClientHTTPState::WaitingForHeaders),
    _clientIP(std::string(clientIP)),
    _clientPort(std::to_string(clientPort)),
    response(nullptr),
    request() {}

Response *Client::_createResponseFromRequest(SocketFD &fd, Request &request) {
    DEBUG("Creating response from request for Client, fd: " << fd.get());

    ServerConfig &config = _server.loadRequestConfig(request, _serverFd);
    const LocationRule &route = config.getLocation(request.metadata.getRawUrl());

    DEBUG("Route found for request: " << route);
    DEBUG("URL path: " << request.metadata.getPath().str());

    if (!route.methods.isAllowed(request.metadata.getMethod()))
        return _createErrorResponse(HttpStatusCode::MethodNotAllowed, route);
    
    if ((route.maxBodySize.isSet() && request.contentLength > route.maxBodySize.getMaxBodySize().get()))
        return _createErrorResponse(HttpStatusCode::PayloadTooLarge, route);

    if (route.returnRule.isSet())
        return _createReturnRuleResponse(route.returnRule);
    
    if (!(route.root.isSet() || route.alias.isSet()))
        return _createErrorResponse(HttpStatusCode::NotFound, route);
    
    request.metadata.translateUrl(_server.getServerExecutablePath(), route);
    if (!request.metadata.getPath().isValid())
        return _createErrorResponse(HttpStatusCode::BadRequest, route);
        
    if (route.cgi.isEnabled() || (!request.metadata.pathIsDirectory() && route.cgiExtension.isCGI(request.metadata.getPath())))
        return _createCGIResponse(fd, config, route);
    
    if (request.metadata.pathIsDirectory())
        return _createDirectoryListingResponse(route);

    if (request.metadata.getMethod() != Method::GET)
        return _createErrorResponse(HttpStatusCode::BadRequest, route);

    int fileFd = open(request.metadata.getPath().str().c_str(), O_RDONLY);
    if (fileFd == -1)
        return _createErrorResponse(HttpStatusCode::NotFound, route);

    return (_configureResponse(new FileResponse(ReadableFD::file(fileFd)), HttpStatusCode::OK));
}

void Client::handleRead(SocketFD &fd, ssize_t funcReturnValue) {
    switch (_state) {
        case ClientHTTPState::WaitingForHeaders: {
            std::string headerString = fd.extractHeadersFromReadBuffer();
            if (headerString.empty()) return ;

            request = Request(headerString);
            response = _createResponseFromRequest(fd, request);

            _state = ClientHTTPState::ReadingBody;
            return handleRead(fd, funcReturnValue);
        }

        case ClientHTTPState::ReadingBody: {
            response->handleRequestBody(fd);
            if (response->isFullRequestBodyRecieved()) {
                _state = ClientHTTPState::SwitchingToOutput;
                return handleRead(fd, funcReturnValue);
            }
            return ;
        }

        case ClientHTTPState::SwitchingToOutput: {
            if (fd.setEpollEvents(EPOLLOUT) == -1) {
                ERROR("Failed to set EPOLLOUT for client: " << fd.get());
                response->terminateResponse();
                _server.untrackClient(fd);
                return ;
            }

            return ;
        }

        default: {
            DEBUG("Client is in an unexpected state: " << static_cast<int>(_state));
            return;
        }
    }
}

void Client::handleWrite(SocketFD &fd) {
    DEBUG("Handling write for Client, fd: " << fd.get());

    if (!response) {
        ERROR("No response to write for Client: " << fd.get());
        return;
    }

    ERROR_IF_NOT(_state == ClientHTTPState::SwitchingToOutput || _state == ClientHTTPState::SendingResponse,
        "Client is not in a valid state for writing: " << static_cast<int>(_state));
    _state = ClientHTTPState::SendingResponse;

    response->handleSocketWriteTick(fd);

    if (response->isFullResponseSent())
        handleClientReset(fd);
}

void Client::handleClientReset(SocketFD &fd) {
    DEBUG("Full response sent for Client, fd: " << fd.get());
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

    _state = ClientHTTPState::WaitingForHeaders;
    request = Request();
}

void Client::switchResponseToErrorResponse(HttpStatusCode statusCode) {
    DEBUG("SWITCHING response to error response for Client: " << _clientIP << ":" << _clientPort);
    DEBUG("Current response: " << (response ? "exists" : "does not exist"));
    DEBUG("Response address: " << response);
    if (response) {
        DEBUG("Deleting existing response for Client: " << _clientIP << ":" << _clientPort);
        delete response;
    }

    ServerConfig &config = _server.loadRequestConfig(request, _serverFd);
    response = _createErrorResponse(statusCode, config.getLocation(request.metadata.getRawUrl()));
}

Client::~Client() {
    DEBUG("Destroying client for IP: " << _clientIP << ", Port: " << _clientPort << " (" << this << ")");
    if (response) {
        if (!response->isFullResponseSent())
            response->terminateResponse();
        delete response;
    }
}
