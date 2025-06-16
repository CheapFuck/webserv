#include "rules.hpp"
#include <iostream>

PortRule::PortRule() : _port(-1) {}

PortRule::PortRule(const Rules &rules, bool required) {
	if (rules.empty() && required)
		throw ConfigParsingException("Missing port rule");
	
	if (rules.size() > 1)
		throw ConfigParsingException("Multiple port rules found");

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ConfigParsingException("Invalid port rule");
		if (rule.arguments[0].type != STRING)
			throw ConfigParsingException("Invalid port argument type");

		size_t char_count = 0;
		_port = std::stoi(rule.arguments[0].str, &char_count);
		if (char_count != rule.arguments[0].str.size())
			throw ConfigParsingException("Invalid port value: " + rule.arguments[0].str);
		if (_port < 1 || _port > 65535)
			throw ConfigParsingException("Port number out of range: " + rule.arguments[0].str);
	}
}

inline int PortRule::get() const {
	return _port;
}

std::ostream& operator<<(std::ostream& os, const PortRule& rule) {
	os << "PortRule(port: " << rule.get() << ")";
	return os;
}