#include "parserExceptions.hpp"
#include "rules/ruleParser.hpp"
#include "rules/rules.hpp"
#include "../print.hpp"
#include "config.hpp"

#include <memory>

static Key getRuleKeyFromToken(Token *token) {
    static const std::map<std::string, Key> keyMap = {
        {ServerConfig::getRuleName(), ServerConfig::getKey()},
        {PortRule::getRuleName(), PortRule::getKey()},
        {LocationRule::getRuleName(), LocationRule::getKey()},
        {ServerNameRule::getRuleName(), ServerNameRule::getKey()},
        {MaxBodySizeRule::getRuleName(), MaxBodySizeRule::getKey()},
        {ErrorPageRule::getRuleName(), ErrorPageRule::getKey()},
        {RootRule::getRuleName(), RootRule::getKey()},
        {IndexRule::getRuleName(), IndexRule::getKey()},
        {AutoIndexRule::getRuleName(), AutoIndexRule::getKey()},
        {ReturnRule::getRuleName(), ReturnRule::getKey()},
        {MethodsRule::getRuleName(), MethodsRule::getKey()},
        {UploadStoreRule::getRuleName(), UploadStoreRule::getKey()},
        {CgiRule::getRuleName(), CgiRule::getKey()},
        {CgiTimeoutRule::getRuleName(), CgiTimeoutRule::getKey()},
        {CgiExtensionRule::getRuleName(), CgiExtensionRule::getKey()},
        {DefineRule::getRuleName(), DefineRule::getKey()},
        {IncludeRule::getRuleName(), IncludeRule::getKey()},
        {AliasRule::getRuleName(), AliasRule::getKey()},
    };

    auto it = keyMap.find(token->value);
    if (it == keyMap.end())
        throw ParserTokenException("Unknown rule key \"" + token->value + "\"", token);
    return (it->second);
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

Object *Object::deepCopy(Arena &arena, Rule *newParentRule) const {
    Object *newObject = arena.alloc<Object>(newParentRule, objectOpenToken, objectCloseToken);

    for (const auto &[key, rulesVec] : rules) {
        Rules newRules;
        newRules.reserve(rulesVec.size());
        for (Rule *rule : rulesVec)
            newRules.push_back(rule->deepCopy(arena, newObject));
        newObject->rules[key] = std::move(newRules);
    }
    return (newObject);
}

Argument *Argument::deepCopy(Arena &arena, Rule *newParentRule) const {
    Argument *newArg = arena.alloc<Argument>(type, value, newParentRule, token);
    if (type == ArgumentType::OBJECT)
        newArg->value = std::get<Object*>(value)->deepCopy(arena, newParentRule);
    return (newArg);
}

Rule *Rule::deepCopy(Arena &arena, Object *newParentObject) const {
    Rule *newRule = arena.alloc<Rule>(key, std::vector<Argument*>(), newParentObject, std::vector<Rule*>(includeRuleRefs), token, isUsed);
    newRule->arguments.reserve(arguments.size());

    for (const Argument *arg : arguments)
        newRule->arguments.push_back(arg->deepCopy(arena, newRule));
    
    return (newRule);
}

void ConfigurationParser::_handleDefineRule(Rule *rule) {
    DefineRule defineRule(rule);

    if (_objects.find(defineRule.getName()) != _objects.end())
        throw ParserArgumentException("Object with name '" + defineRule.getName() + "' already exists", rule->arguments[0],
                "Give the object a different (unique) name to avoid conflicts.");
    if (defineRule.getName() == rule->token->configFile->fileName)
        throw ParserArgumentException("Object name cannot be the same as the configuration file name", rule->arguments[0],
            "Choose a different name for the object to avoid conflicts with the configuration file name.");
    _objects[defineRule.getName()] = defineRule.getObject();
}

void ConfigurationParser::_includeObjectIntoScope(Object *object, Object *includedObject, Rule *includeRuleRef) {
    for (const auto &[key, rules] : includedObject->rules) {
        if (object->rules.find(key) == object->rules.end())
            object->rules[key] = {};
        for (Rule *rule : rules) {
            Rule *newRule = rule->deepCopy(_arena, object);
            newRule->includeRuleRefs.push_back(includeRuleRef);
            newRule->parentObject = object;
            object->rules[key].push_back(newRule);
        }
    }
}

void ConfigurationParser::_handleIncludeRule(ConfigFile *file, size_t &pos, Rule *rule, Object *object) {
    IncludeRule includeRule(rule);

    if (!isFileLoaded(includeRule.getIncludePath())) {
        try { _loadConfigFile(includeRule.getIncludePath()); }
        catch (ParserException &e) {
            e.addTracebackFromRule(rule);
            throw;
        }
    }

    auto it = _objects.find(includeRule.getIncludePath());
    if (it == _objects.end())
        throw ParserTokenException("Included object '" + includeRule.getIncludePath() + "' not found in the configuration", file->tokens[pos]);
    _includeObjectIntoScope(object, it->second, rule);
}

Rule *ConfigurationParser::_parseRule(ConfigFile *file, size_t &pos, Object *parentObject) {
    if (file->tokens[pos]->type != TokenType::WEAK_STR)
        throw ParserTokenException("Expected a rule key, but found something else", file->tokens[pos]);

    Token *ruleToken = file->tokens[pos++];
    Rule *rule = _arena.alloc<Rule>(getRuleKeyFromToken(ruleToken), std::vector<Argument*>(), parentObject, std::vector<Rule *>(), ruleToken, false);

    while (true) {
        if (file->tokens[pos]->type == TokenType::RULE_END) {
            ++pos;
            break;
        }

        else if (file->tokens[pos]->type == TokenType::OBJECT_OPEN) {
            rule->arguments.push_back(_arena.alloc<Argument>(ArgumentType::OBJECT, _parseObject(file, pos, rule), rule, file->tokens[pos]));
            if (file->tokens[pos]->type == TokenType::RULE_END)
                ++pos;
            break;
        }

        else if (file->tokens[pos]->type == TokenType::WEAK_STR || file->tokens[pos]->type == TokenType::STR) {
            if (file->tokens[pos]->type == TokenType::WEAK_STR) {
                Keyword keyword = getKeyword(file->tokens[pos]->value);
                if (keyword != Keyword::NO_KEYWORD) {
                    rule->arguments.push_back(_arena.alloc<Argument>(ArgumentType::KEYWORD, keyword, rule, file->tokens[pos]));
                    ++pos;
                    continue;
                }
            }
            rule->arguments.push_back(_arena.alloc<Argument>(ArgumentType::STRING, file->tokens[pos]->value, rule, file->tokens[pos]));
            ++pos;
        }

        else if (file->tokens[pos]->type == TokenType::STR || file->tokens[pos]->type == TokenType::WEAK_STR) {
            rule->arguments.push_back(_arena.alloc<Argument>(ArgumentType::STRING, file->tokens[pos]->value, rule, file->tokens[pos]));
            ++pos;
        }

        else
            throw ParserTokenException("Unexpected token type while parsing rule", file->tokens[pos]);
    }

    return (rule);
}

Object *ConfigurationParser::_parseObject(ConfigFile *file, size_t &pos, Rule *parentRule) {
    Object *object = _arena.alloc<Object>(parentRule, file->tokens[pos++], nullptr);

    while (file->tokens[pos]->type != TokenType::OBJECT_CLOSE) {
        Rule *rule = _parseRule(file, pos, object);
        if (rule->key == Key::DEFINE)
            _handleDefineRule(rule);
        else if (rule->key == Key::INCLUDE)
            _handleIncludeRule(file, pos, rule, object);
        else {
            auto it = object->rules.find(rule->key);
            if (it == object->rules.end())
                object->rules[rule->key] = Rules();
            object->rules[rule->key].push_back(rule);
        }
    }

    object->objectCloseToken = file->tokens[pos++];
    return (object);
}

Object *ConfigurationParser::_getObjectFromFile(ConfigFile *file) {
    size_t pos = 0;

    return (_parseObject(file, pos, nullptr));
}
