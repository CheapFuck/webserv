#include "config/rules/rules.hpp"
#include "response.hpp"
#include "methods.hpp"
#include "print.hpp"
#include "Utils.hpp"
#include "redact_dir_listing.cpp"

#include <sys/stat.h>

/// @brief Tries to create a response from an index rule by checking the specified
/// index files in the given directory. The first match file will be used to create
/// the response.
static bool _tryCreateResponseFromIndex(
    Response &response,
    const Path &filepath,
    const IndexRule &index_rule
) {
    for (const auto &index_page : index_rule.get()) {
        Path index_path = filepath;
        index_path.append(index_page);
        if (tryCreateResponseFromFile(index_path, response)) {
            DEBUG("Response created from index file: " << index_path.str());
            response.setStatusCode(HttpStatusCode::OK);
            return (true);
        }
    }
    return (false);
}

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
    responseBody = redactDirectoryListing(filepath.str(), request.metadata.getPath());
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
    if (route.index.isSet()) {
        if (_tryCreateResponseFromIndex(response, filepath, route.index))
            return (response);
        DEBUG("No index file found in directory: " << filepath.str());
    }

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
    DEBUG("Processing GET request for path: " << request.metadata.getPath());

    if (!route.root.isSet()) {
        DEBUG("Root not set for route: " << route);
        response.setStatusCode(HttpStatusCode::InternalServerError);
        return response;
    }

    Path filepath = Path::createFromUrl(request.metadata.getPath(), route);
    if (!filepath.isValid()) {
        DEBUG("Invalid request path: " << filepath.str());
        response.setStatusCode(HttpStatusCode::BadRequest);
        return response;
    }

    PathStat path_stat{};
    if (stat(filepath.str().c_str(), &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
        DEBUG("Request path is a directory: " << filepath.str());
        return handleDirectoryRequest(request, response, route, filepath);
    }

    // Handle regular file requests (CGI will be handled by server before this method is called)
    if (!tryCreateResponseFromFile(filepath, response)) {
        DEBUG("File not found: " << filepath.str());
        response.setStatusCode(HttpStatusCode::NotFound);
    }

    return response;
}