#include "rules.hpp"

#include <string.h>
#include <iostream>

ServerNameRule::ServerNameRule() : _server_name() {}

ServerNameRule::ServerNameRule(const Rules &rules, bool required) {
	if (rules.empty() && required)
		throw ConfigParsingException("Missing server name rule");
	
	if (rules.size() > 1)
		throw ConfigParsingException("Multiple server name rules found");

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ConfigParsingException("Invalid server name rule");
		if (rule.arguments[0].type != STRING)
			throw ConfigParsingException("Invalid server name argument type");

		_server_name = std::string(rule.arguments[0].str);
	}
}

inline std::string ServerNameRule::get() const {
	return _server_name;
}

std::ostream& operator<<(std::ostream& os, const ServerNameRule& rule) {
	os << "ServerNameRule(server_name: " << rule.get() << ")";
	return os;
}
