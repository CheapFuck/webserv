#include "methods.hpp"
#include "client.hpp"
#include "print.hpp"

Client::Client(const Client &other) 
    : request(other.request), response(other.response) {}

Client &Client::operator=(const Client &other) {
    if (this != &other) {
        request = other.request;
        response = other.response;
    }
    return *this;
}

/// @brief Reads data from the specified file descriptor into the request buffer.
/// @param fd The file descriptor to read from.
/// @return Returns false if the connection is closed, true otherwise.
bool Client::read(int fd) {
    return request.read(fd);
}

/// @brief Checks if the request is complete and ready for processing.
/// @return Returns true if the request is complete, false otherwise.
bool Client::requestIsComplete() const {
    return request.isComplete();
}

/// @brief Sends the response to the client.
/// @return Returns true if the response was sent successfully, false otherwise.
bool Client::sendResponse(int fd, ServerConfig& using_conf) {
    response.setDefaultBody(using_conf);
    return response.sendToClient(fd);
}

/// @brief Resets the client state, clearing the request and response objects.
void Client::reset() {
    request = Request();
    response = Response();
    DEBUG("Client reset");
}

/// @brief Processes the request based on its HTTP method.
/// @param config The server configuration
/// @param route The location rule that matches the request path
/// @return Returns a reference to the response object after processing the request.
Response &Client::_processRequestByMethod(const ServerConfig &config, const LocationRule &route) {
    (void)config;
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
Response &Client::processRequest(const ServerConfig &config) {
    DEBUG("Processing client request");
    response.setDefaultHeaders();

    if (!request.isComplete()) {
        response.setStatusCode(HttpStatusCode::BadRequest);
        return (response);
    }

    const LocationRule *route = config.routes.find(request.metadata.getPath());
    if (route == nullptr || request.headers.getHeader(HeaderKey::Host) != config.serverName.get()) {
        response.setStatusCode(HttpStatusCode::NotFound);
        return (response);
    }
RequestLine copy = request.metadata;
    DEBUG("Route found: " << *route);
    // DEBUG("Url Path: " << request.metadata);
    DEBUG("Url Path: " << copy);

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
        response.setStatusCode(HttpStatusCode::MovedPermanently);
        response.headers.replace(HeaderKey::Location, route->redirect.get());
        return (response);
    }

    return _processRequestByMethod(config, *route);
}
