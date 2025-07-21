#pragma once

#include "config/rules/rules.hpp"
#include "response.hpp"
#include "request.hpp"

#include <sys/stat.h>
#include <string>

typedef struct stat PathStat;

bool tryCreateResponseFromFile(const Path &filepath, Response &response);
std::string getMimeType(const std::string &filename);

class GetMethod {
private:
    GetMethod() = default;
    ~GetMethod() = default;

public:
    static Response &processRequest(
        const Request &request,
        Response &response,
        const ServerConfig &config,
        const LocationRule &route
    );
};

class PostMethod {
private:
    PostMethod() = default;
    ~PostMethod() = default;

public:
    struct FilePart {
        std::string name;
        std::string filename;
        std::string contentType;
        std::string content;
    };

    static Response &processRequest(
        const Request &request,
        Response &response,
        const ServerConfig &config,
        const LocationRule &route
    );
};

class DeleteMethod {
private:
    DeleteMethod() = default;
    ~DeleteMethod() = default;

public:
    static Response &processRequest(
        const Request &request,
        Response &response,
        const LocationRule &route
    );
};
