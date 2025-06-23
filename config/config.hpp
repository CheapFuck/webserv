#pragma once

#include "consts.hpp"

#include <exception>
#include <string>
#include <vector>
#include <map>

class 	ServerConfig;
struct	ErrorContext;
struct	Rule;

enum TokenType {
	UNKNOWN_TOKEN = 0,
	STR = 1 << 0,
	BRACE_OPEN = 1 << 1,
	BRACE_CLOSE = 1 << 2,
	LINE_END = 1 << 3,
	WHITESPACE = 1 << 4,
	COMMENT = 1 << 5,
	END = 1 << 6,
};

enum Keyword {
	NO_KEYWORD = 0,
	ON = 1 << 0,
	OFF = 1 << 1,
	TRUE = 1 << 2,
	FALSE = 1 << 3,
	DEFAULT = 1 << 4,
	ENABLE = 1 << 5,
	DISABLE = 1 << 6,
	AUTO = 1 << 7,
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
	CGI_TIMEOUT = 1 << 13,
	DEFINE = 1 << 14,
	INCLUDE = 1 << 15,
};

#define IS_USABLE_TOKEN(type) ((type) & (STR | BRACE_OPEN | BRACE_CLOSE | LINE_END | KEYWORD | END))

struct Token {
	TokenType type;
	std::string value;
	int filePos;

	int line;
    Token(TokenType t, const std::string& v, int l)
    : type(t), value(v), line(l) {}
};

typedef std::vector<Rule> Rules;

struct Object {
	std::map<Key, std::vector<Rule>> children;
	int fileObjOpenPos;
	int fileObjClosePos;
};

enum ArgumentType {
	STRING = 1 << 0,
	OBJECT = 1 << 1,
	KEYWORD = 1 << 2,
};

struct Argument {
	ArgumentType type;
	std::string str;
	Keyword keyword;
	Object rules;
	int filePos;
};

struct Rule {
	Key key;
	int filePos;
	std::vector<Argument> arguments;
};

struct LexerContext {
	std::vector<std::pair<std::string, Object>> includes; 
};

class ConfigurationParser {
private:
	std::string _filename;
	std::string _configuration;
	std::vector<size_t> _lineNumbers;
	std::vector<Token> _tokens;
	std::map<int, std::vector<ServerConfig>> _result;

	void _readFile();
	std::vector<Token> _tokenize() const;
	void _fetchServerConfigs(Object &object);
	Object _parseTokens(std::vector<Token> &tokens) const;


public:
	ConfigurationParser(const std::string &filename);
	ConfigurationParser(const ConfigurationParser &other) = default;
	ConfigurationParser &operator=(const ConfigurationParser &other) = default;
	~ConfigurationParser() = default;

	bool fetchConfiguration();
	std::map<int, std::vector<ServerConfig>> getResult() const;

	ErrorContext getErrorContext(size_t pos) const;

	inline const std::string& getFilename() const { return _filename; }
};

struct ErrorContext {
	size_t lineNum;
	size_t columnNum;
	std::string line;
};

class ParserTokenException : public std::exception {
private:
	std::string _message;
	int _filePos;

public:
	ParserTokenException(const std::string &message, const Rule &rule);
	ParserTokenException(const std::string &message, const Token &token);
	ParserTokenException(const std::string &message, const Argument &argument);
	std::string describe(ConfigurationParser &parser) const noexcept;
};

class ParserDuplicationException : public std::exception {
private:
	std::string _message;
	int _firstFilePos;
	int _secondFilePos;

public:
	ParserDuplicationException(const std::string &message, const Rule &firstRule, const Rule &secondRule);
	std::string describe(ConfigurationParser &parser) const noexcept;
};

class ParserMissingException : public std::exception {
private:
	std::string _message;
	int _firstFilePos;
	int _secondFilePos;

public:
	ParserMissingException(const std::string &message);
	std::string describe(ConfigurationParser &parser) const noexcept;

	void attachObject(const Object &object);
	inline bool isObjectAttached() const noexcept { return _firstFilePos != 0 && _secondFilePos != 0; };
};

std::ostream& operator<<(std::ostream& os, const TokenType& type);
std::ostream& operator<<(std::ostream& os, const Token& token);
std::ostream& operator<<(std::ostream& os, const Rule& rule);
std::ostream& operator<<(std::ostream& os, const Object& object);
