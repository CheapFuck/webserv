#include "config.hpp"

#include <iostream>

static Rule parse_rule(std::vector<Token> &tokens, size_t &i, LexerContext &context);
static Object parse_object(std::vector<Token> &tokens, size_t &i, TokenType end_token, LexerContext &context);

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
	for (const auto &[key, rules] : object) {
		os << "Key: " << key << ", Rules: [";
		for (const auto &rule : rules) {
			os << rule;
		}
		os << "]}";
		if (std::next(object.find(key)) != object.end()) os << ", ";
	}
	os << "])";
	return os;
}

static Key getRuleKey(const Token &token) {
	static const std::map<std::string, Key> keyMap = {
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
		{"cgi", CGI_PASS},
		{"cgi_timeout", CGI_TIMEOUT},
		{"define", DEFINE},
		{"include", INCLUDE},
	};

	auto it = keyMap.find(token.value);
	if (it == keyMap.end())
		throw ConfigParsingException("Unknown key: " + token.value);
	return it->second;
}

static Keyword getKeyword(const std::string &str) {
	static const std::map<std::string, Keyword> keywordMap = {
		{"on", ON},
		{"off", OFF},
		{"true", TRUE},
		{"false", FALSE},
		{"default", DEFAULT},
		{"enable", ENABLE},
		{"disable", DISABLE},
		{"auto", AUTO},
	};

	auto it = keywordMap.find(str);
	if (it == keywordMap.end())
		return (NO_KEYWORD);
	return it->second;
}

static void handleNewDefine(const Rule &rule, LexerContext &context) {
	if (rule.arguments.size() != 2 || rule.arguments[0].type != STRING || rule.arguments[1].type != OBJECT)
		throw ConfigParsingException("Invalid DEFINE rule format");
	context.includes.push_back({rule.arguments[0].str, rule.arguments[1].rules});
}

static void handleInclude(Object &rules, const Rule &rule, LexerContext &context) {
	if (rule.arguments.size() != 1 || rule.arguments[0].type != STRING)
		throw ConfigParsingException("Invalid INCLUDE rule format");

	const std::string &includeFile = rule.arguments[0].str;
	for (const auto &[name, includedObject] : context.includes) {
		if (name == includeFile) {
			for (const auto &[key, includedRules] : includedObject) {
				auto it = rules.find(key);
				if (it != rules.end())
					it->second.insert(it->second.end(), includedRules.begin(), includedRules.end());
				else
					rules[key] = {includedRules};
			}
			return;
		}
	}

	throw ConfigParsingException("Include rule not found: " + includeFile);
}

static Object parse_object(std::vector<Token> &tokens, size_t &i, TokenType end_token, LexerContext &context) {
	Object rules;

	while (tokens[i].type != end_token) {
		Rule rule = parse_rule(tokens, i, context);
		if (rule.key == DEFINE)
			handleNewDefine(rule, context);
		else if (rule.key == INCLUDE)
			handleInclude(rules, rule, context);
		else {
			auto it = rules.find(rule.key);
			if (it != rules.end())
				it->second.push_back(rule);
			else 
				rules[rule.key] = {rule};
		}
	}

	++i;
	return rules;
}

static Rule parse_rule(std::vector<Token> &tokens, size_t &i, LexerContext &context) {
	if (tokens[i].type != STR)
		throw ConfigParsingException("Expected a string token");

	Rule rule {getRuleKey(tokens[i++]), {}};

	while (true) {
		if (tokens[i].type == SEMICOLON) {
			++i;
			break;
		}

		else if (tokens[i].type == BRACE_OPEN) {
			++i;
			rule.arguments.push_back({OBJECT, "", NO_KEYWORD, parse_object(tokens, i, BRACE_CLOSE, context)});
			break;
		}

		else if (tokens[i].type == STR) {
			Keyword keyword = getKeyword(tokens[i].value);
			if (keyword != NO_KEYWORD)
				rule.arguments.push_back({KEYWORD, std::string(tokens[i].value), keyword, {}});
			else
				rule.arguments.push_back({STRING, std::string(tokens[i].value), {}, {}});
			++i;
		}

		else throw ConfigParsingException("Wrongly formatted rule");
	}

	return rule;
}

Object lexer(std::vector<Token> &tokens) {
	LexerContext context;
	size_t i = 0;

	return parse_object(tokens, i, END, context);
}
