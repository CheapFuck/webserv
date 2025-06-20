#include "config.hpp"

#include <iostream>

static Rule parse_rule(std::vector<Token> &tokens, size_t &i, LexerContext &context);
static Object parse_object(std::vector<Token> &tokens, size_t &i, LexerContext &context);

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
	for (const auto &[key, rules] : object.children) {
		os << "Key: " << key << ", Rules: [";
		for (const auto &rule : rules) {
			os << rule;
		}
		os << "]}";
		if (std::next(object.children.find(key)) != object.children.end()) os << ", ";
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
		throw ParserTokenException("Unknown rule key: " + token.value, token);
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
		throw ParserTokenException("Invalid DEFINE rule format", rule);
	context.includes.push_back({rule.arguments[0].str, rule.arguments[1].rules});
}

static void handleInclude(Object &rules, const Rule &rule, LexerContext &context) {
	if (rule.arguments.size() != 1 || rule.arguments[0].type != STRING)
		throw ParserTokenException("Invalid INCLUDE rule format", rule);

	const std::string &includeFile = rule.arguments[0].str;
	for (const auto &[name, includedObject] : context.includes) {
		if (name == includeFile) {
			for (const auto &[key, includedRules] : includedObject.children) {
				auto it = rules.children.find(key);
				if (it != rules.children.end())
					it->second.insert(it->second.end(), includedRules.begin(), includedRules.end());
				else
					rules.children[key] = {includedRules};
			}
			return;
		}
	}

	throw ParserTokenException("The include named " + includeFile + " is not (yet) defined", rule.arguments[0]);
}

static Object parse_object(std::vector<Token> &tokens, size_t &i, LexerContext &context) {
	Object rules{{}, tokens[i++].filePos, -1};

	while (tokens[i].type != BRACE_CLOSE) {
		Rule rule = parse_rule(tokens, i, context);
		if (rule.key == DEFINE)
			handleNewDefine(rule, context);
		else if (rule.key == INCLUDE)
			handleInclude(rules, rule, context);
		else {
			auto it = rules.children.find(rule.key);
			if (it != rules.children.end())
				it->second.push_back(rule);
			else 
				rules.children[rule.key] = {rule};
		}
	}

	rules.fileObjClosePos = tokens[i++].filePos;
	return (rules);
}

static Rule parse_rule(std::vector<Token> &tokens, size_t &i, LexerContext &context) {
	if (tokens[i].type != STR)
		throw ParserTokenException("Expected a rule or closing bracket", tokens[i]);

	Token &ruleToken = tokens[i++];
	Rule rule {getRuleKey(ruleToken), ruleToken.filePos, {}};

	while (true) {
		if (tokens[i].type == LINE_END) {
			++i;
			break;
		}

		else if (tokens[i].type == BRACE_OPEN) {
			rule.arguments.push_back({OBJECT, "", NO_KEYWORD, parse_object(tokens, i, context), tokens[i].filePos});
			break;
		}

		else if (tokens[i].type == STR) {
			Keyword keyword = getKeyword(tokens[i].value);
			if (keyword != NO_KEYWORD)
				rule.arguments.push_back({KEYWORD, std::string(tokens[i].value), keyword, {}, tokens[i].filePos});
			else
				rule.arguments.push_back({STRING, std::string(tokens[i].value), {}, {}, tokens[i].filePos});
			++i;
		}

		else throw ParserTokenException("Wrongly formatted rule", tokens[i]);
	}

	return rule;
}

Object ConfigurationParser::_parseTokens(std::vector<Token> &tokens) const {
	LexerContext context;
	size_t i = 0;

	return parse_object(tokens, i, context);
}
