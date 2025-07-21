#include "config/rules/rules.hpp"
#include "response.hpp"
#include "methods.hpp"
#include "print.hpp"
#include "Utils.hpp"
#include "redact_dir_listing.cpp"

#include <sys/stat.h>

/// @brief Handles a directory listing request by creating a response with a
/// directory listing HTML page.
static Response &_handleDirectoryListing(
    const Request &request,
    Response &response,
    const Path &filepath
) {
    std::string responseBody;
    DEBUG("Handling directory listing for: " << filepath.str());
    response.setStatusCode(HttpStatusCode::OK);
    response.headers.replace(HeaderKey::ContentType, "text/html");
    responseBody = redactDirectoryListing(filepath.str(), request.metadata.getRawUrl());
    response.setBody(responseBody);
    return (response);
}

/// @brief Handles a GET request for a directory.
static Response &handleDirectoryRequest(
    const Request &request,
    Response &response,
    const LocationRule &route,
    const Path &filepath
) {
    if (route.autoIndex.get())
        return (_handleDirectoryListing(request, response, filepath));
    DEBUG("Autoindex not enabled for directory: " << filepath.str());
    response.setStatusCode(HttpStatusCode::Forbidden);
    return (response);
}

/// @brief Processes a GET request
Response &GetMethod::processRequest(
    const Request &request,
    Response &response,
    const ServerConfig &config,
    const LocationRule &route
) {
    (void)config; // Suppress unused variable warning
    DEBUG("Processing GET request for path: " << request.metadata.getRawUrl());

    if (request.metadata.pathIsDirectory())
        return handleDirectoryRequest(request, response, route, request.metadata.getPath());

    if (!tryCreateResponseFromFile(request.metadata.getPath(), response)) {
        DEBUG("File not found: " << request.metadata.getPath().str());
        response.setStatusCode(HttpStatusCode::NotFound);
    }

    return response;
}