#include "rules.hpp"
#include "../config.hpp"

#include <iostream>

CGIRule::CGIRule(const Object &obj, bool required) : _cgi_paths({}) {
	for (const Rule &rule : obj) {
		if (rule.key != CGI_PASS) continue;

		if (rule.arguments.size() != 2)
			throw ConfigParsingException("Invalid cgi_pass rule");
		if (rule.arguments[0].type != STRING)
			throw ConfigParsingException("Invalid cgi_pass argument type");
		if (rule.arguments[1].type != STRING)
			throw ConfigParsingException("Invalid cgi_pass argument type");

		_cgi_paths[rule.arguments[0].str] = rule.arguments[1].str;
	}

	if (_cgi_paths.empty() && required)
		throw ConfigParsingException("Missing cgi_pass rule");
}

const std::map<std::string, std::string> &CGIRule::getPaths() const {
	return _cgi_paths;
}

/// @brief Get the path of the interpreter
/// @param ext The file extension for which to get the CGI interpreter path
/// @return A constant reference to the path of the CGI interpreter for the given extension
const std::string &CGIRule::getPath(const std::string &ext) const {
	std::map<std::string, std::string>::const_iterator iter = _cgi_paths.find(ext);
	if (iter != _cgi_paths.end())
		return iter->second;
	throw ConfigParsingException("CGI path not found for extension: " + ext);
}

/// @brief Check if the CGI rule is set
bool CGIRule::isSet() const {
	return !_cgi_paths.empty();
}

/// @brief Check if a CGI interpreter exists
/// @param ext The file extension to check for a CGI interpreter
bool CGIRule::exists(const std::string &ext) const {
	return _cgi_paths.find(ext) != _cgi_paths.end();
}

std::ostream& operator<<(std::ostream& os, const CGIRule& rule) {
	os << "CGIRule(cgi_paths: {";
	for (const auto &pair : rule.getPaths()) {
		os << pair.first << ": " << pair.second << ", ";
	}
	os << "})";
	return os;
}
