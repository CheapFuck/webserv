#include "client.hpp"
#include "methods.hpp"
#include "server.hpp"
#include "print.hpp"
#include "fd.hpp"

Client::Client(Server &server, int serverFd, const char *clientIP, int clientPort) : _cgiClient(nullptr), _CGIWriteFd(nullptr), _server(server), _serverFd(serverFd), _clientIP(std::string(clientIP)), _clientPort(std::to_string(clientPort)) {}

Response &Client::_createCGIClient(const ServerConfig &config, const LocationRule &route) {
    _cgiClient = std::make_shared<CGIClient>(*this);
    if (!_cgiClient) {
        ERROR("Failed to create CGIClient for request processing");
        response.setStatusCode(HttpStatusCode::InternalServerError);
        return (response);
    }
    _cgiClient->start(config, route);
    return (response);
}

/// @brief Processes the request based on its HTTP method.
/// @param config The server configuration
/// @param route The location rule that matches the request path
/// @return Returns a reference to the response object after processing the request.
Response &Client::_processRequestByMethod(const ServerConfig &config, const LocationRule &route) {
    (void)config;

    if (route.cgi.isSet() || (!request.metadata.pathIsDirectory() && route.cgiExtension.isCGI(request.metadata.getPath())))
        return _createCGIClient(config, route);

    switch (request.metadata.getMethod()) {
        case Method::GET:
            return GetMethod::processRequest(request, response, config, route);
        case Method::POST:
            return PostMethod::processRequest(request, response, config, route);
        case Method::DELETE:
            return DeleteMethod::processRequest(request, response, route);
        default:
            response.setStatusCode(HttpStatusCode::MethodNotAllowed);
            return response;
    }
}

/// @brief Handles the return rule response.
/// @param returnRule The return rule to handle
/// @return Returns a reference to the response object after handling the return rule.
Response &Client::_handleReturnRuleResponse(const ReturnRule &returnRule) {
    DEBUG("Handling return rule response: " << returnRule);
    response.setStatusCode(returnRule.getStatusCode());
    if (returnRule.isRedirect()) {
        response.headers.replace(HeaderKey::Location, returnRule.getParameter());
        DEBUG("Redirecting to: " << returnRule.getParameter());
    } else {
        response.setBody(returnRule.getParameter());
        DEBUG("Returning body: " << returnRule.getParameter());
    }
    return response;
}

/// @brief Processes the request.
/// @param config The server configuration
Response &Client::_processRequest(const ServerConfig &config) {
    DEBUG("Processing client request");
    _server.fetchUserSession(request, response);
    response.setDefaultHeaders();

    if (!request.isComplete(request.getBody())) {
        response.setStatusCode(HttpStatusCode::BadRequest);
        return (response);
    }

    const LocationRule &route = config.getLocation(request.metadata.getRawUrl());

    DEBUG("Route found: " << route);
    DEBUG("Url Path: " << request.metadata);

    if (!route.methods.isAllowed(request.metadata.getMethod())) {
        response.setStatusCode(HttpStatusCode::MethodNotAllowed);
        return (response);
    }

    if ((route.maxBodySize.isSet() && request.getContentLength() > route.maxBodySize.getMaxBodySize().get())
        || request.getBody().length() > route.maxBodySize.getMaxBodySize().get()) {
        response.setStatusCode(HttpStatusCode::PayloadTooLarge);
        DEBUG("Request body exceeds maximum size");
        return response;
    }

    if (route.returnRule.isSet())
        return _handleReturnRuleResponse(route.returnRule);

    if (!route.root.isSet()) {
        DEBUG("Root not set for route: " << route);
        response.setStatusCode(HttpStatusCode::NotFound);
        return response;
    }

    request.metadata.translateUrl(_server.getServerExecutablePath(), route);
    DEBUG("Translated request path: " << request.metadata.getPath().str());

    if (!request.metadata.getPath().isValid()) {
        DEBUG("Invalid request path: " << request.metadata.getPath().str());
        response.setStatusCode(HttpStatusCode::BadRequest);
        return (response);
    }

    return _processRequestByMethod(config, route);
}

void Client::handleReadCallback(FD &fd, int funcReturnValue) {
    DEBUG("Read callback for Client, fd: " << fd.get() << ", funcReturnValue: " << funcReturnValue);
    (void)funcReturnValue;

    request.parseRequestHeaders(fd.readBuffer);
    if (request.isComplete(fd.readBuffer)) {
        request.setBody(fd.readBuffer);
        _processRequest(_server.loadRequestConfig(request, _serverFd));
        DEBUG("Request is complete, processing request for fd: " << fd.get());
        if (fd.setEpollOut() == -1) {
            ERROR("Failed to set EPOLLOUT for Client: " << fd.get());
            _server.untrackDescriptor(fd.get());
            return;
        }
    }
}

void Client::handleCGIResponse() {
    DEBUG("Handling CGI response for Client, fd: " << (_CGIWriteFd ? _CGIWriteFd->get() : -1));
    if (!_CGIWriteFd) return ;
    DEBUG("Fire response");
    handleWriteCallback(*_CGIWriteFd);
}

void Client::handleWriteCallback(FD &fd) {
    DEBUG("Write callback for Client, fd: " << fd.get());
    if (_cgiClient && _cgiClient->isRunning()) {
        _CGIWriteFd = &fd;
        return ;
    }

    const LocationRule &rule = _server.loadRequestConfig(request, _serverFd).getLocation(request.metadata.getRawUrl());
    response.setDefaultBody(rule);
    
    // if (request.headers.getHeader(HeaderKey::Connection) == "close") {
    //     response.headers.replace(HeaderKey::Connection, "close");
    // }

    fd.writeToBuffer(response.getAsString());
    
    if (fd.write() == -1) {
        ERROR("Failed to write response for Client: " << fd.get());
        _server.untrackDescriptor(fd.get());
        return ;
    }

    fd.readBuffer.clear();
    fd.writeBuffer.clear();
    _CGIWriteFd = nullptr;

    if (request.headers.getHeader(HeaderKey::Connection, "keep-alive") == "close") {
        DEBUG("Connection header indicates 'close', disconnecting Client: " << fd.get());
        _server.untrackDescriptor(fd.get());
        return ;
    } else if (fd.setEpollIn() == -1) {
        ERROR("Failed to set EPOLLIN for Client: " << fd.get());
        _server.untrackDescriptor(fd.get());
    }

    request = Request();
    response = Response();
}

void Client::handleDisconnectCallback(FD &fd) {
    DEBUG("Disconnect callback for Client, fd: " << fd.get());
    (void)fd;

    if (_cgiClient && _cgiClient->isRunning()) {
        ERROR("CGIClient is still running, exiting process");
        _cgiClient->exitProcess();
    }
}
