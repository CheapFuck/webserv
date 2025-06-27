#include "config/rules/rules.hpp"
#include "response.hpp"
#include "methods.hpp"
#include "print.hpp"

#include <filesystem>
#include <sstream>
#include <string>
#include <map>

static std::map<std::string, std::string> parseQueryParams(const std::string& url) {
    std::map<std::string, std::string> params;
    size_t queryStart = url.find("?");
    
    if (queryStart == std::string::npos)
        return (params);

    std::string queryString = url.substr(queryStart + 1);
    std::istringstream queryStream(queryString);
    std::string param;

    while (std::getline(queryStream, param, '&')) {
        size_t eqPos = param.find("=");
        if (eqPos != std::string::npos) {
            std::string key = param.substr(0, eqPos);
            std::string value = param.substr(eqPos + 1);
            params[key] = value;
        }
    }

    return (params);
}

Response &DeleteMethod::processRequest(
    const Request &request,
    Response &response,
    const LocationRule &route
) {
    DEBUG("Processing DELETE request for path: " << request.metadata.getRawUrl());
    std::map<std::string, std::string> queryParams = parseQueryParams(request.metadata.getRawUrl());

    response.headers.replace(HeaderKey::ContentType, "text/plain");

    if (queryParams.find("file") == queryParams.end()) {
        response.setStatusCode(HttpStatusCode::BadRequest);
        response.setBody("Missing 'file' query parameter");
        return (response);
    }

    if (!route.upload_dir.isSet()) {
        response.setStatusCode(HttpStatusCode::InternalServerError);
        response.setBody("Upload directory not configured");
        return (response);
    }

    Path filepath(route.upload_dir.get());
    filepath.append(queryParams["file"]);
    if (!filepath.isValid()) {
        response.setStatusCode(HttpStatusCode::BadRequest);
        response.setBody("Invalid file path");
        return (response);
    }

    if (!std::filesystem::exists(filepath.str())) {
        response.setStatusCode(HttpStatusCode::NotFound);
        response.setBody("File not found");
        return (response);
    }

    if (std::remove(filepath.str().c_str())) {
        response.setStatusCode(HttpStatusCode::InternalServerError);
        response.setBody("Failed to delete file");
        return (response);
    }

    response.setStatusCode(HttpStatusCode::OK);
    response.setBody("File deleted successfully");
    return (response);
}
