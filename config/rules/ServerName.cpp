#include "rules.hpp"

#include <string.h>
#include <iostream>

ServerNameRule::ServerNameRule(const Object &obj, bool required) {
	int count = 0;

	for (const Rule &rule : obj) {
		if (rule.key != SERVER_NAME) continue;

		if (rule.arguments.size() != 1)
			throw ConfigParsingException("Invalid server name rule");
		if (rule.arguments[0].type != STRING)
			throw ConfigParsingException("Invalid server name argument type");
		if (++count > 1)
			throw ConfigParsingException("Duplicate server name rule");

		_server_name = std::string(rule.arguments[0].str);
	}

	if (count == 0 && required)
		throw ConfigParsingException("Missing server name rule");
}

inline std::string ServerNameRule::get() const {
	return _server_name;
}

std::ostream& operator<<(std::ostream& os, const ServerNameRule& rule) {
	os << "ServerNameRule(server_name: " << rule.get() << ")";
	return os;
}
