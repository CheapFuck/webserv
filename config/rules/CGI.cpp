#include "rules.hpp"
#include "../config.hpp"

#include <iostream>

CGIRule::CGIRule() : _cgiPaths({}) {}

CGIRule::CGIRule(const Rules &rules, bool required) : _cgiPaths({}) {
	if (rules.empty() && required)
		throw ConfigParsingException("Missing cgi_pass rule");

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 2)
			throw ConfigParsingException("Invalid cgi_pass rule");
		if (rule.arguments[0].type != STRING)
			throw ConfigParsingException("Invalid cgi_pass argument type");
		if (rule.arguments[1].type != STRING)
			throw ConfigParsingException("Invalid cgi_pass argument type");

		_cgiPaths[rule.arguments[0].str] = rule.arguments[1].str;
	}
}

/// @brief  @brief Update the CGI paths from a default rule
/// @param defaultRule The default CGI rule to update from
void CGIRule::updateFromDefault(const CGIRule &defaultRule) {
	if (defaultRule.isSet()) {
		if (_cgiPaths.empty()) {
			_cgiPaths = defaultRule._cgiPaths;
		} else {
			for (const auto &pair : defaultRule._cgiPaths) {
				if (_cgiPaths.find(pair.first) == _cgiPaths.end()) {
					_cgiPaths[pair.first] = pair.second;
				}
			}
		}
	}
}

/// @brief Get the CGI paths
/// @return A constant reference to the map of CGI paths
const std::map<std::string, std::string> &CGIRule::getPaths() const {
	return _cgiPaths;
}

/// @brief Get the path of the interpreter
/// @param ext The file extension for which to get the CGI interpreter path
/// @return A constant reference to the path of the CGI interpreter for the given extension
const std::string &CGIRule::getPath(const std::string &ext) const {
	std::map<std::string, std::string>::const_iterator iter = _cgiPaths.find(ext);
	if (iter != _cgiPaths.end())
		return iter->second;
	throw ConfigParsingException("CGI path not found for extension: " + ext);
}

/// @brief Check if the CGI rule is set
bool CGIRule::isSet() const {
	return !_cgiPaths.empty();
}

/// @brief Check if a CGI interpreter exists
/// @param ext The file extension to check for a CGI interpreter
bool CGIRule::exists(const std::string &ext) const {
	return _cgiPaths.find(ext) != _cgiPaths.end();
}

std::ostream& operator<<(std::ostream& os, const CGIRule& rule) {
	os << "CGIRule(cgi_paths: {";
	for (const auto &pair : rule.getPaths()) {
		os << pair.first << ": " << pair.second << ", ";
	}
	os << "})";
	return os;
}
