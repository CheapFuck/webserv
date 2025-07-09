#include "../rules/rules.hpp"
#include "../../print.hpp"
#include "customTypes.hpp"

#include <filesystem>
#include <stdexcept>

Path::Path(const std::string &str) : _path(std::string(str)) {
	_is_set = true;
}

Path::Path(const Path &other) : _path(other._path), _is_set(other._is_set) {}

Path::Path() : _path(""), _is_set(false) {}

Path &Path::operator=(const Path &other) {
	if (this != &other) {
		_path = other._path;
		_is_set = other._is_set;
	}
	return *this;
}

/// @brief Return a dummy Path object (which is not set)
Path Path::createDummy() {
	return Path();
}

/// @brief  Append a subpath to the current path
/// @param str The subpath to append
/// @return A reference to itself for chaining
/// @details Appending an absolute path (starting with '/') will replace the current path.
Path &Path::append(const std::string &str) {
	if (str.empty()) return *this;
	if (_path.empty() || str.front() == '/') {
		_path = std::string(str);
		return *this;
	} else {
		if (_path.back() == '/') {
			_path += str;
		} else {
			_path += '/' + str;
		}
		return *this;
	}
}

/// @brief Update the path from a URL and a root directory
/// @param route The URL route to update from
/// @param root The root directory to use for the update
/// @return A reference to itself for chaining
/// @details Expects the path to be initialized with the URL route.
/// It replaces the beginning of the path with the root directory.
Path &Path::updateFromUrl(const std::string &route, const std::string &root) {
	size_t root_len = root.length();

	_path.replace(0, route.length(), root);
	if (_path[root_len] && _path[root_len] != '/')
		_path.insert(root_len, "/");
	if (_path.back() == '/') _path.pop_back();

	return *this;
}

/// @brief Remove the last segment of the path - either a file or a directory.
/// @return A reference to itself for chaining
Path &Path::pop() {
	if (_path.empty()) return *this;

	size_t last_slash = _path.find_last_of('/');
	if (last_slash == std::string::npos) {
		_path.clear();
	} else {
		_path.erase(last_slash);
	}

	return *this;
}

/// @brief Create a Path object from a URL and a route.
/// @param url The URL to create the path from.
/// @param route The rules for the URL
/// @return A path to the file or directory represented by the URL.
Path Path::createFromUrl(const std::string &url, const LocationRule &route) {
	if (url.empty() || !route.root.isSet())
		return Path::createDummy();

	Path path(url.substr(0, url.find('?')));
	path.updateFromUrl(route.path.str(), route.root.getRootPath().str());

	return path;
}

/// @brief Get the filename from the path. If the path is a directory, it returns the last segment.
/// @return The filename or the last segment of the path.
std::string Path::getFilename() const {
	if (_path.empty()) return "";

	size_t last_slash = _path.find_last_of('/');
	if (last_slash == std::string::npos)
		return _path;
	else
		return _path.substr(last_slash + 1);
}

/// @brief Check if the path is valid. For this it has to be set and not contain ".." segments. 
bool Path::isValid() const {
	return (_is_set && _path.find("..") == std::string::npos);
}

/// @brief Get the string representation of the path.
/// @return A constant reference to the path string.
const std::string &Path::str() const {
	return _path;
}

std::ostream &operator<<(std::ostream &os, const Path &path) {
	os << "Path(path: " << path.str() << ", isValid: " << path.isValid() << ")";
	return os;
}
