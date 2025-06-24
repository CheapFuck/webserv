#include "../config.hpp"
#include "rules.hpp"
#include "../../print.hpp"

#include <iostream>

CGIExtentionRule::CGIExtentionRule() : _extentions() {}

CGIExtentionRule::CGIExtentionRule(const Rules &rules, bool required) : _extentions() {
	if (rules.empty() && required)
		throw ParserMissingException("Missing cgi_extention rule");

	if (rules.size() > 1)
		throw ParserDuplicationException("Multiple cgi_extention rules found", rules[0], rules[1]);

	for (const Rule &rule : rules) {
		if (!rule.arguments.size())
			throw ParserTokenException("Invalid cgi rule format", rule);

		for (const Argument &arg : rule.arguments) {
			if (arg.type != STRING)
				throw ParserTokenException("Invalid cgi argument type", arg);
			if (arg.str.size() < 2 || arg.str[0] != '.' || std::find_if(arg.str.begin() + 1, arg.str.end(), [](char c) { return !std::isalnum(c) && c != '_'; }) != arg.str.end())
				throw ParserTokenException("Invalid cgi extension: " + arg.str, arg);
			_extentions.push_back(arg.str);
		}
	}
}

/// @brief Checks if the given path has a CGI extension.
/// @param path The path to check against -> assumes the path is to a file; without any extra arguments.
/// @return True if the path has a CGI extension, false otherwise.
bool CGIExtentionRule::isCGI(const Path &path) const {
	std::string filename = path.getFilename();
	size_t lastDotPos = filename.find_last_of('.', filename.find_first_of('?'));
	if (lastDotPos == std::string::npos || lastDotPos == filename.size() - 1)
		return (false);
	std::string ext = filename.substr(lastDotPos);
	return (std::find(_extentions.begin(), _extentions.end(), ext) != _extentions.end());
}

/// @brief Returns the list of CGI extensions defined in this rule.
/// @return A constant reference to the vector of CGI extensions.
const std::vector<std::string> &CGIExtentionRule::getExtentions() const {
	return (_extentions);
}

std::ostream& operator<<(std::ostream& os, const CGIExtentionRule& rule) {
	os << "CGIExtentionRule(";
	for (const auto &ext : rule.getExtentions()) {
		os << ext << " ";
	}
	os << ")";
	return os;
}
