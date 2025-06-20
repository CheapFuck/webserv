#include "../../print.hpp"
#include "rules.hpp"

#include <functional>
#include <iostream>
#include <map>

ServerConfig::ServerConfig(Object &object) {
	defaultLocation = LocationRule("", object, false);

	std::unordered_map<Key, std::function<void(Rules &)>> ruleParsers = {
		{LISTEN, [this](const Rules &rules) { port = PortRule(rules, true); }},
		{SERVER_NAME, [this](const Rules &rules) { serverName = ServerNameRule(rules, false); }},
		{LOCATION, [this](Rules &rules) { routes = RouteRules(rules, defaultLocation, false); }}
	};

	extractRules(object, ruleParsers, true);
}

/// @brief Fetches server configurations from the given object.
/// @param object The root object returned by the lexer
/// @return A vector of ServerConfig objects
std::vector<ServerConfig> fetchServerConfigs(Object &object) {
	std::vector<ServerConfig> serverConfigs;

	std::unordered_map<Key, std::function<void(Rules &)>> ruleParsers = {
		{SERVER, [&serverConfigs](Rules &rules) { 
			for (Rule & rule : rules) {
				if (rule.arguments.size() != 1)
					throw ParserTokenException("Invalid server rule format", rule);
				if (rule.arguments[0].type != OBJECT)
					throw ParserTokenException("Invalid server rule argument type", rule.arguments[0]);
				serverConfigs.push_back(ServerConfig(rule.arguments[0].rules));
			}
		 }},
	};

	extractRules(object, ruleParsers, true);
	return (serverConfigs);
}

std::ostream& operator<<(std::ostream& os, const ServerConfig& config) {
	os << config.port << std::endl;
	os << config.serverName << std::endl;
	os << config.routes << std::endl;
	os << "Default Location: " << config.defaultLocation << std::endl;

	return os;
}