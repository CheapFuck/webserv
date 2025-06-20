#include "config.hpp"

#include <iostream>
#include <cstring>
#include <fstream>

std::ostream& operator<<(std::ostream& os, const TokenType& type) {
	switch (type) {
		case STR: os << "STR"; break;
		case BRACE_OPEN: os << "BRACE_OPEN"; break;
		case BRACE_CLOSE: os << "BRACE_CLOSE"; break;
		case LINE_END: os << "LINE_END"; break;
		case WHITESPACE: os << "WHITESPACE"; break;
		case COMMENT: os << "COMMENT"; break;
		case END: os << "END"; break;
		default: os << "UNKNOWN"; break;
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
	os << "Token(type: " << token.type << ", value: \"" << token.value << "\")";
	return os;
}

static TokenType getTokenType(const char *str, size_t &i, bool advance = true) {
	static const std::pair<const char *, TokenType> tokens[] = {
		{"{",  BRACE_OPEN},
		{"}", BRACE_CLOSE},
		{";", LINE_END},
		{"#", COMMENT},
	};

	for (auto &token : tokens) {
		if (std::strncmp(str + i, token.first, std::strlen(token.first)) == 0) {
			if (advance) i += strlen(token.first);
			return token.second;
		}
	}

	if (str[i] == '\0')
		return END;

	if (std::isspace(str[i]))
	{
		if (advance) while (std::isspace(str[i])) ++i;
		return WHITESPACE;
	}

	if (advance) ++i;
	return STR;
}

static std::string getWhitespaceTokenValue(const char *str, size_t &i) {
	while (getTokenType(str, i, false) == WHITESPACE) ++i;
	return "";
}

static std::string getCommentTokenValue(const char *str, size_t &i) {
	size_t start = i;
	while (str[i] && str[i] != '\n') ++i;
	return (std::string(str + start, (i++) - start));
}

static std::string getStringTokenValue(const char *str, size_t &i) {
	size_t start = i - 1;
	while (getTokenType(str, i, false) == STR) ++i;
	return (std::string(str + start, i - start));
}

static Token get_next_token(const char *str, size_t &i) {
	int filePos = i;
	TokenType type = getTokenType(str, i);

	switch (type) {
		case WHITESPACE:
			return Token{type, getWhitespaceTokenValue(str, i), filePos};
		case COMMENT:
			return Token{type, getCommentTokenValue(str, i), filePos};
		case STR:
			return Token{type, getStringTokenValue(str, i), filePos};
		case BRACE_OPEN:
		case BRACE_CLOSE:
		case LINE_END:
		case END:
		default:
			return Token{type, "", filePos};
	}
}

std::vector<Token> ConfigurationParser::_tokenize() const {
	std::vector<Token> tokens = {Token(BRACE_OPEN, "", 0)};
	size_t i = 0;

	while (true) {
		Token token = get_next_token(_configuration.c_str(), i);
		if (token.type == END)
			tokens.push_back(Token(BRACE_CLOSE, "", i));
		if (IS_USABLE_TOKEN(token.type))
			tokens.push_back(token);
		if (token.type == END)
			break;
	}

	return tokens;
}
