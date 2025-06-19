#include "client.hpp"
#include "methods.hpp"
#include "server.hpp"
#include "print.hpp"
#include "fd.hpp"

Client::Client(Server &server, int serverFd, std::string &clientIP, std::string &clientPort) : _server(server), _serverFd(serverFd), _clientIP(clientIP), _clientPort(clientPort), _cgiClient(nullptr) {}

/// @brief Processes the request based on its HTTP method.
/// @param config The server configuration
/// @param route The location rule that matches the request path
/// @return Returns a reference to the response object after processing the request.
Response &Client::_processRequestByMethod(const ServerConfig &config, const LocationRule &route) {
    (void)config;

    if (route.CGI.isSet()) {
        _cgiClient = std::make_shared<CGIClient>(*this);
        if (!_cgiClient) {
            ERROR("Failed to create CGIClient for request processing");
            response.setStatusCode(HttpStatusCode::InternalServerError);
            return (response);
        }
        _cgiClient->start(config, route);
        return (response);
    }

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

/// @brief Processes the request.
/// @param config The server configuration
Response &Client::_processRequest(const ServerConfig &config) {
    DEBUG("Processing client request");
    response.setDefaultHeaders();

    if (!request.isComplete(request.getBody())) {
        response.setStatusCode(HttpStatusCode::BadRequest);
        return (response);
    }

    const LocationRule *route = config.routes.find(request.metadata.getPath());
    if (route == nullptr) {
        response.setStatusCode(HttpStatusCode::NotFound);
        return (response);
    }

    DEBUG("Route found: " << *route);
    DEBUG("Url Path: " << request.metadata);

    if (!route->methods.isAllowed(request.metadata.getMethod())) {
        response.setStatusCode(HttpStatusCode::MethodNotAllowed);
        return (response);
    }

    if (request.getContentLength() > config.clientMaxBodySize.get()
        || request.getBody().length() > config.clientMaxBodySize.get()) {
        response.setStatusCode(HttpStatusCode::PayloadTooLarge);
        DEBUG("Request body exceeds maximum size");
        return response;
    }

    if (route->redirect.isSet()) {
        DEBUG("Redirecting to: " << route->redirect.get());
        response.setStatusCode(HttpStatusCode::MovedPermanently);
        response.headers.replace(HeaderKey::Location, route->redirect.get());
        return (response);
    }

    return _processRequestByMethod(config, *route);
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
            fd.close();
            return;
        }
    }
}

void Client::handleWriteCallback(FD &fd) {
    DEBUG("Write callback for Client, fd: " << fd.get());

    response.setDefaultBody(_server.loadRequestConfig(request, _serverFd));
    fd.writeToBuffer(response.getAsString());

    if (fd.write() == -1) {
        ERROR("Failed to write response for Client: " << fd.get());
        fd.close();
        return;
    }

    if (fd.setEpollIn() == -1) {
        ERROR("Failed to set EPOLLIN for Client: " << fd.get());
        fd.close();
        return;
    }

    request = Request();
    response = Response();
    fd.readBuffer.clear();
    fd.writeBuffer.clear();
}

void Client::handleDisconnectCallback(FD &fd) {
    DEBUG("Disconnect callback for Client, fd: " << fd.get());
    (void)fd;
}
