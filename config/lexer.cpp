#include "config.hpp"

#include <iostream>

static Rule parse_rule(std::vector<Token> &tokens, size_t &i);
static Object parse_object(std::vector<Token> &tokens, size_t &i, TokenType end_token);

const char* ConfigParsingException::what() const noexcept {
	return _message.c_str();
}

std::ostream& operator<<(std::ostream& os, const Rule& rule) {
	os << "Rule(key: " << rule.key << ", arguments: [";
	for (size_t i = 0; i < rule.arguments.size(); ++i) {
		if (rule.arguments[i].type == STRING)
			os << rule.arguments[i].str;
		else if (rule.arguments[i].type == OBJECT)
			os << rule.arguments[i].rules;
		else
			os << "Unknown argument type";
		if (i < rule.arguments.size() - 1) os << ", ";
	}
	os << "])";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Object& object) {
	os << "Object(rules: [";
	for (size_t i = 0; i < object.size(); ++i) {
		os << object[i];
		if (i < object.size() - 1) os << ", ";
	}
	os << "])";
	return os;
}

static Key get_rule_key(const std::string &str) {
	static const std::map<std::string, Key> key_map = {
		{"server", SERVER},
		{"listen", LISTEN},
		{"server_name", SERVER_NAME},
		{"client_max_body_size", MAX_BODY_SIZE},
		{"error_page", ERROR_PAGE},
		{"root", ROOT},
		{"index", INDEX},
		{"autoindex", AUTOINDEX},
		{"redirect", REDIRECT},
		{"location", LOCATION},
		{"allowed_methods", ALLOWED_METHODS},
		{"upload_store", UPLOAD_DIR},
		{"cgi_pass", CGI_PASS}
	};

	auto it = key_map.find(str);
	if (it == key_map.end())
		throw ConfigParsingException("Unknown key: " + str);
	return it->second;
}

static Object parse_object(std::vector<Token> &tokens, size_t &i, TokenType end_token) {
	Object rules;

	while (tokens[i].type != end_token)
		rules.push_back(parse_rule(tokens, i));

	++i;
	return rules;
}

static Rule parse_rule(std::vector<Token> &tokens, size_t &i) {
	if (tokens[i].type != STR)
		throw ConfigParsingException("Expected a string token");

	Rule rule {get_rule_key(tokens[i++].value), {}, false};

	while (true) {
		if (tokens[i].type == SEMICOLON) {
			++i;
			break;
		}

		else if (tokens[i].type == BRACE_OPEN) {
			++i;
			rule.arguments.push_back({OBJECT, "", parse_object(tokens, i, BRACE_CLOSE)});
			break;
		}

		else if (tokens[i].type == STR) {
			rule.arguments.push_back({STRING, std::string(tokens[i].value), {}});
			++i;
		}

		else throw ConfigParsingException("Wrongly formatted rule");
	}

	return rule;
}

Object lexer(std::vector<Token> &tokens) {
	size_t i = 0;
	return parse_object(tokens, i, END);
}
