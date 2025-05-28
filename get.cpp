#include "config/rules/rules.hpp"
#include "response.hpp"
#include "methods.hpp"
#include "print.hpp"
#include "Utils.hpp"
#include "CGI.hpp"
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
    Response &response,
    const Path &filepath
) {
	std::string responseBody;
    //TODO: Implement directory listing logic
    DEBUG("Handling directory listing for: " << filepath.str());
    response.setStatusCode(HttpStatusCode::OK);
    response.headers.replace(HeaderKey::ContentType, "text/html");
    responseBody = redactDirectoryListing(filepath.str());
    response.setBody(responseBody);
    return (response);
}

/// @brief Handles a GET request for a directory.
static Response &handleDirectoryRequest(
    Response &response,
    const LocationRule &route,
    const Path &filepath
) {
    if (route.index.isSet()) {
        if (_tryCreateResponseFromIndex(response, filepath, route.index))
            return (response);
        DEBUG("No index file found in directory: " << filepath.str());
    }

    if (route.autoindex.get())
        return (_handleDirectoryListing(response, filepath));
    DEBUG("Autoindex not enabled for directory: " << filepath.str());
    response.setStatusCode(HttpStatusCode::Forbidden);
    return (response);
}

static bool tryCgiExecution(
    const Request &request,
    const LocationRule &route,
    const ServerConfig &config,
    Response &response
) {
    std::string ext = Utils::getFileExtension(request.metadata.getPath());
    if (!route.cgi_paths.exists(ext)) {
        DEBUG("No CGI interpreter found for extension: " << ext);
        return (false);
    }

    DEBUG("CGI interpreter found for extension: " << ext);
    std::string interpreter = route.cgi_paths.getPath(ext);
    DEBUG("Using CGI interpreter: " << interpreter);

    CGI cgi;
    if (cgi.execute(request, route, config, response)) {
        DEBUG("CGI execution successful");
        return (true);
    } else {
        DEBUG("CGI execution failed");
        response.setStatusCode(HttpStatusCode::InternalServerError);
        return (true);
    }
}

/// @brief Processes a GET request
Response &GetMethod::processRequest(
    const Request &request,
    Response &response,
    const ServerConfig &config,
    const LocationRule &route
) {
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
        return handleDirectoryRequest(response, route, filepath);
    } else if (route.cgi_paths.isSet() && tryCgiExecution(request, route, config, response))
        return (response);

    if (!tryCreateResponseFromFile(filepath, response)) {
        DEBUG("File not found: " << filepath.str());
        response.setStatusCode(HttpStatusCode::NotFound);
    }

    return (response);
}
