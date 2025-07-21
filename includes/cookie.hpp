#pragma once

#include "config/rules/rules.hpp"
#include "request.hpp"

#include <iostream>
#include <vector>
#include <string>

class Request;

class Cookie {
private:
	std::string _name;
	std::string _value;
	Path _path;
	size_t _maxAge;

public:
	Cookie() = default;
	Cookie(const Cookie &other) = default;
	Cookie &operator=(const Cookie &other) = default;
	~Cookie() = default;

	static Cookie create(const std::string &name, const std::string &value);
	static std::vector<Cookie> createAllFromHeader(const std::string &header_value);

	inline Cookie &setPath(const Path &path) {
		_path = path;
		return *this;
	}

	inline Cookie &setMaxAge(size_t maxAge) {
		_maxAge = maxAge;
		return *this;
	}

	inline Cookie &setName(const std::string &name) {
		_name = name;
		return *this;
	}

	inline Cookie &setValue(const std::string &value) {
		_value = value;
		return *this;
	}

	inline const std::string &getName() const { return _name; }
	inline const std::string &getValue() const { return _value; }
	inline const Path &getPath() const { return _path; }
	inline size_t getMaxAge() const { return _maxAge; }

	std::string getHeaderInitializationString() const;

	static Cookie createSessionCookie(const std::string &sessionId);
	static const Cookie *getCookie(const std::vector<Cookie> &cookies, const std::string &name);
	static const Cookie *getCookie(const Request &request, const std::string &name);
};
