#include "rules.hpp"
#include <iostream>

ServerConfig::ServerConfig(Object &object)
	: port(PortRule(object, true)),
	  server_name(ServerNameRule(object, true)),
	  client_max_body_size(MaxBodySizeRule(object, true)),
	  error_pages(ErrorPageRule(object, false)),
	  routes(RouteRules(object, true)) {}

/// @brief Fetches server configurations from the given object.
/// @param object The root object returned by the lexer
/// @return A vector of ServerConfig objects
std::vector<ServerConfig> fetchServerConfigs(Object &object) {
	std::vector<ServerConfig> server_configs;
	server_configs.reserve(object.size());

	for (size_t i = 0; i < object.size(); ++i) {
		if (object[i].key != SERVER ||
			object[i].arguments.size() != 1 ||
			object[i].arguments[0].type != OBJECT)
			throw ConfigParsingException("Invalid server configuration");
		server_configs.push_back(ServerConfig(object[i].arguments[0].rules));
	}
	return server_configs;
}

std::ostream& operator<<(std::ostream& os, const ServerConfig& config) {
	os << config.port << std::endl;
	os << config.server_name << std::endl;
	os << config.client_max_body_size << std::endl;
	os << config.error_pages << std::endl;
	os << config.routes << std::endl;

	return os;
}