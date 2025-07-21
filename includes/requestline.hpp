#pragma once

#include "config/types/customTypes.hpp"
#include "config/types/consts.hpp"
#include "config/rules/rules.hpp"

#include <sstream>
#include <string>

typedef struct stat PathStat;

class RequestLine {
private:
    Method _method;
    std::string _url;
    std::string _version;

    Path _path;
    Path _serverAbsolutePath;
    bool _pathIsDirectory;

    bool _fetchCorrectPathFromIndexRule(const IndexRule &rule);

public:
    RequestLine();
    RequestLine(std::istringstream &source);
    RequestLine(const RequestLine &other);
    RequestLine &operator=(const RequestLine &other);
    ~RequestLine();

    void translateUrl(const std::string &serverRelativePath, const LocationRule &route);

    bool isValid() const;

    const Method &getMethod() const;
    const std::string &getRawUrl() const;
    const std::string &getVersion() const;
    const std::string &getServerAbsolutePath() const;
    const Path &getPath() const;
    bool pathIsDirectory() const;
};

std::ostream &operator<<(std::ostream &os, const RequestLine &request_line);