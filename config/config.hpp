#pragma once

#include "consts.hpp"

#include <exception>
#include <string>
#include <vector>
#include <map>

class ConfigParsingException : public std::exception {
public:
	ConfigParsingException(const std::string& message) : _message(message) {}
	const char* what() const noexcept;
private:
	std::string _message;
};

struct	Rule;

enum TokenType {
	UNKNOWN_TOKEN = 0,
	STR = 1 << 0,
	BRACE_OPEN = 1 << 1,
	BRACE_CLOSE = 1 << 2,
	SEMICOLON = 1 << 3,
	WHITESPACE = 1 << 4,
	COMMENT = 1 << 5,
	END = 1 << 6,
};

#define IS_USABLE_TOKEN(type) ((type) & (STR | BRACE_OPEN | BRACE_CLOSE | SEMICOLON | END))

struct Token {
	TokenType type;
	std::string value;
};

enum Key {
	SERVER = 1 << 0,
	LISTEN = 1 << 1,
	SERVER_NAME = 1 << 2,
	MAX_BODY_SIZE = 1 << 3,
	ERROR_PAGE = 1 << 4,
	ROOT = 1 << 5,
	INDEX = 1 << 6,
	AUTOINDEX = 1 << 7,
	REDIRECT = 1 << 8,
	LOCATION = 1 << 9,
	ALLOWED_METHODS = 1 << 10,
	UPLOAD_DIR = 1 << 11,
	CGI_PASS = 1 << 12,
};

typedef std::map<Key, std::vector<Rule>> Object;
typedef std::vector<Rule> Rules;

enum ArgumentType {
	STRING = 1 << 0,
	OBJECT = 1 << 1,
};

struct Argument {
	ArgumentType type;
	std::string str;
	Object rules;
};

struct Rule {
	Key key;
	std::vector<Argument> arguments;
};

std::ostream& operator<<(std::ostream& os, const TokenType& type);
std::ostream& operator<<(std::ostream& os, const Token& token);
std::ostream& operator<<(std::ostream& os, const Rule& rule);
std::ostream& operator<<(std::ostream& os, const Object& object);

std::vector<Token> tokenize(const std::string& filename);

Object lexer(std::vector<Token> &tokens);
