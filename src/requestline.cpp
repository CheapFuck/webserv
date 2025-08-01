#include "requestline.hpp"
#include "print.hpp"
#include "Utils.hpp"

#include <sys/stat.h>
#include <sstream>

static int hexCharToInt(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    return -1; // invalid hex char
}

static std::string urlDecode(const std::string& str) {
    std::string result;
    for (std::size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int high = hexCharToInt(str[i + 1]);
            int low = hexCharToInt(str[i + 2]);
            if (high != -1 && low != -1) {
                char decodedChar = static_cast<char>((high << 4) | low);
                result += decodedChar;
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

RequestLine::RequestLine() : _method(UNKNOWN_METHOD), _url(), _version("HTTP/1.1"), _path(), _serverAbsolutePath(Path::createDummy()), _pathIsDirectory(false) {}

RequestLine::RequestLine(std::istringstream &source) 
    : _path(), _serverAbsolutePath(Path::createDummy()), _pathIsDirectory(false) {
    std::string method_str;
    source >> method_str >> _url >> _version;

    _url = urlDecode(Utils::trim(_url));
    _version = Utils::trim(_version);
    _method = stringToMethod(Utils::trim(method_str));
}

RequestLine::RequestLine(const RequestLine &other)
    : _method(other._method), _url(other._url), _version(other._version), _path(other._path), _serverAbsolutePath(other._serverAbsolutePath), _pathIsDirectory(other._pathIsDirectory) {}

RequestLine &RequestLine::operator=(const RequestLine &other) {
    if (this != &other) {
        _method = other._method;
        _url = other._url;
        _version = other._version;
        _path = other._path;
        _serverAbsolutePath = other._serverAbsolutePath;
        _pathIsDirectory = other._pathIsDirectory;
    }
    return *this;
}

RequestLine::~RequestLine() {}

/// @brief Fetches the correct path from the index rule.
/// @param rule The index rule containing the list of index pages.
/// @return Returns true if a valid index file was found and set as the path, false otherwise.
bool RequestLine::_fetchCorrectPathFromIndexRule(const IndexRule &rule) {
    PathStat pathStat{};

    for (const auto &indexPage : rule.getIndexFiles()) {
        Path indexPath = _path;
        indexPath.append(indexPage);

        if (stat(indexPath.str().c_str(), &pathStat) == 0) {
            DEBUG("Using index file: " << _path.str());
            _path = indexPath;
            return (true);
        }
    }

    return (false);
}

/// @brief Translates the URL of the request line to a Path object based on the given route. If the path is a directory
/// and the index rule is set, it will try to fetch the correct path from the index rule.
/// @param route The location rule that contains the routing information for the request.
void RequestLine::translateUrl(const std::string &serverRelativePath, const LocationRule &route) {
    _path = Path(serverRelativePath);
    _path.append(Path::createFromUrl(_url, route).str());
    _serverAbsolutePath = Path(serverRelativePath);

    PathStat pathStat{};
    if (stat(_path.str().c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
        _pathIsDirectory = !(route.index.isSet() && _fetchCorrectPathFromIndexRule(route.index));
    } else {
        _pathIsDirectory = false;
    }
}

/// @brief Checks if the request line is valid.
bool RequestLine::isValid() const {
    return (_method != UNKNOWN_METHOD && !_url.empty() && !_version.empty());
}

/// @brief Returns the HTTP method of the request line.
const Method &RequestLine::getMethod() const {
    return _method;
}

/// @brief Returns the path (url) of the request line.
const std::string &RequestLine::getRawUrl() const {
    return _url;
}

/// @brief Returns the HTTP version of the request line.
const std::string &RequestLine::getVersion() const {
    return _version;
}

/// @brief Returns the server absolute path derived from the request line's URL.
const std::string &RequestLine::getServerAbsolutePath() const {
    return _serverAbsolutePath.str();
}

/// @brief Returns the local path derived from the request line's URL.
/// This path is used to access the requested resource on the server.
/// If the path is a directory and the index rule is set, it will return the path to the index file.
/// If the path is not a directory, it will return the path to the requested resource.
/// @return A constant reference to the Path object representing the local path.
const Path &RequestLine::getPath() const {
    return _path;
}

/// @brief Checks if the path is a directory.
/// @return Returns true if the path is a directory, false otherwise.
bool RequestLine::pathIsDirectory() const {
    return _pathIsDirectory;
}

std::ostream &operator<<(std::ostream &os, const RequestLine &request_line) {
    os << request_line.getMethod() << " " << request_line.getRawUrl() << " " << request_line.getVersion();
    return os;
}
