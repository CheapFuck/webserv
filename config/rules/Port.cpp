#include "rules.hpp"
#include "../../print.hpp"

#include <iostream>

PortRule::PortRule() : _port(-1) {}

PortRule::PortRule(const Rules &rules, bool required): _port(-1) {
	if (rules.empty() && required)
		throw ParserMissingException("Missing port rule");
	
	if (rules.size() > 1)
		throw ParserDuplicationException("Multiple port rules found", rules[0], rules[1]);

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ParserTokenException("Invalid port rule format", rule);
		if (rule.arguments[0].type != STRING)
			throw ParserTokenException("Invalid port argument type", rule.arguments[0]);

		size_t char_count = 0;
		try {_port = std::stoi(rule.arguments[0].str, &char_count); }
		catch (const std::invalid_argument &e) {
			throw ParserTokenException("Invalid port value: " + rule.arguments[0].str, rule.arguments[0]);
		}
		catch (const std::out_of_range &e) {
			throw ParserTokenException("Port number out of range: " + rule.arguments[0].str, rule.arguments[0]);
		}

		if (char_count != rule.arguments[0].str.size())
			throw ParserTokenException("Invalid port value: " + rule.arguments[0].str, rule.arguments[0]);
		if (_port < 1 || _port > 65535)
			throw ParserTokenException("Port number out of range: " + rule.arguments[0].str, rule.arguments[0]);
	}
}

inline int PortRule::get() const {
	return _port;
}

std::ostream& operator<<(std::ostream& os, const PortRule& rule) {
	os << "PortRule(port: " << rule.get() << ")";
	return os;
}