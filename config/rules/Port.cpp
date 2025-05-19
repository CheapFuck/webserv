#include "rules.hpp"
#include <iostream>

PortRule::PortRule(const Object &obj, bool required) {
	int count = 0;

	for (const Rule &rule : obj) {
		if (rule.key != LISTEN) continue;

		if (rule.arguments.size() != 1)
			throw ConfigParsingException("Invalid port rule");
		if (rule.arguments[0].type != STRING)
			throw ConfigParsingException("Invalid port argument type");
		if (++count > 1)
			throw ConfigParsingException("Duplicate port rule");

		size_t char_count = 0;
		_port = std::stoi(rule.arguments[0].str, &char_count);
		if (char_count != rule.arguments[0].str.size())
			throw ConfigParsingException("Invalid port value: " + rule.arguments[0].str);
		if (_port < 1 || _port > 65535)
			throw ConfigParsingException("Port number out of range: " + rule.arguments[0].str);
	}

	if (count == 0 && required)
		throw ConfigParsingException("Missing port rule");
}

inline int PortRule::get() const {
	return _port;
}

std::ostream& operator<<(std::ostream& os, const PortRule& rule) {
	os << "PortRule(port: " << rule.get() << ")";
	return os;
}