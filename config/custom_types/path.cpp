#include "types.hpp"

#include <filesystem>
#include <stdexcept>

Path::Path(const std::string &str) : _path(std::string(str)) {
	// TODO: Implement some sort of check later
	// if (!std::filesystem::exists(_path))
	// 	throw std::invalid_argument("Invalid path: " + _path);
	_is_valid = true;
}

Path::Path(const Path &other) : _path(other._path), _is_valid(other._is_valid) {}

Path::Path() : _path(""), _is_valid(false) {}

Path &Path::operator=(const Path &other) {
	if (this != &other) {
		_path = other._path;
		_is_valid = other._is_valid;
	}
	return *this;
}

Path Path::create_dummy() {
	return Path();
}

bool Path::is_valid() const {
	return _is_valid;
}

std::string Path::get_path() const {
	return _path;
}

std::ostream &operator<<(std::ostream &os, const Path &path) {
	os << "Path(path: " << path.get_path() << ", is_valid: " << path.is_valid() << ")";
	return os;
}
