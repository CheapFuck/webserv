#include "../../print.hpp"
#include "../rules/rules.hpp"

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

Path Path::create_dummy() {
	return Path();
}

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

Path &Path::update_from_url(const std::string &route, const std::string &root) {
	size_t root_len = root.length();

	_path.replace(0, route.length(), root);
	if (_path[root_len] && _path[root_len] != '/')
		_path.insert(root_len, "/");
	if (_path.back() == '/') _path.pop_back();

	return *this;
}

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

Path Path::create_from_url(const std::string &url, const LocationRule &route) {
	if (url.empty() || !route.root.is_set())
		return Path::create_dummy();

	Path path(url);
	path.update_from_url(route.get_path(), route.root.get().get_path());

	return path;
}

std::string Path::get_filename() const {
	if (_path.empty()) return "";

	size_t last_slash = _path.find_last_of('/');
	if (last_slash == std::string::npos)
		return _path;
	else
		return _path.substr(last_slash + 1);
}

bool Path::is_valid() const {
	return (_is_set && _path.find("..") == std::string::npos);
}

const std::string &Path::get_path() const {
	return _path;
}

std::ostream &operator<<(std::ostream &os, const Path &path) {
	os << "Path(path: " << path.get_path() << ", is_valid: " << path.is_valid() << ")";
	return os;
}
