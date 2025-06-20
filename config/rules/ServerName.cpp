#include "rules.hpp"

#include <string.h>
#include <iostream>

ServerNameRule::ServerNameRule() : _server_name() {}

ServerNameRule::ServerNameRule(const Rules &rules, bool required): _server_name() {
	if (rules.empty() && required)
		throw ParserMissingException("Missing server name rule");
	
	if (rules.size() > 1)
		throw ParserDuplicationException("Multiple server name rules found", rules[0], rules[1]);

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ParserTokenException("Invalid server name rule format", rule);
		if (rule.arguments[0].type != STRING)
			throw ParserTokenException("Invalid server name argument type", rule.arguments[0]);

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
