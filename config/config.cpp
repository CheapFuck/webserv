#include "rules/ruleTemplates/serverconfigRule.hpp"
#include "rules/objectParser.hpp"
#include "parserExceptions.hpp"
#include "../print.hpp"
#include "config.hpp"

#include <fstream>
#include <variant>

ErrorContext::ErrorContext(const std::string &filename, const std::string &line, size_t lineNumber, size_t columnNumber)
    : filename(filename), line(line), lineNumber(lineNumber), columnNumber(columnNumber) {}

Token::Token(TokenType type, std::string value, ConfigFile *configFile, size_t filePos)
    : type(type), value(std::move(value)), configFile(configFile), filePos(filePos) {}

ConfigFile::ConfigFile(const std::string& fileName, std::string fileContent, std::vector<Token*> tokens, std::vector<size_t> lineStarts)
    : fileName(fileName), fileContent(std::move(fileContent)), tokens(std::move(tokens)), lineStarts(std::move(lineStarts)) {}

Object::Object(Rule *parentRule, Token *objectOpenToken, Token *objectCloseToken)
    : rules({}), parentRule(parentRule), objectOpenToken(objectOpenToken), objectCloseToken(objectCloseToken) {}

Rule::Rule(Key key, std::vector<Argument*> arguments, Object *parentObject, std::vector<Rule*> includeRuleRefs, Token *token, bool isUsed)
    : key(key), arguments(std::move(arguments)), parentObject(parentObject), includeRuleRefs(includeRuleRefs), token(token), isUsed(isUsed) {}

Argument::Argument(ArgumentType type, ArgumentValue value, Rule *parentRule, Token *token)
    : type(type), value(std::move(value)), parentRule(parentRule), token(token) {}

/// @brief Recursively checks for unused rules in the configuration object.
/// @param object The configuration object to check for unused rules.
/// @param unusedException The exception to which unused rules will be added.
static void _checkUnusedRules(Object *object, ParserUnusedRuleException *unusedException) {
    for (const auto &[key, rules] : object->rules) {
        for (Rule *rule : rules) {
            if (!rule->isUsed) {
                unusedException->addUnusedRule(rule);
                continue;
            }

            // If the rule is used, we check its arguments for unused rules.
            for (Argument *arg : rule->arguments) {
                if (arg->type == ArgumentType::OBJECT)
                    _checkUnusedRules(std::get<Object *>(arg->value), unusedException);
            }
        }
    }
}

/// @brief Checks for unused rules in the configuration object. Only usable after the configuration has been parsed.
/// @param object The configuration object to check for unused rules.
/// @throws ParserUnusedRuleException if unused rules are found.
static void checkUnusedRules(Object *object) {
    ParserUnusedRuleException exception("Unused rules found in configuration file", \
        "Remove the rules that are not used; or include them in a valid context.");

    _checkUnusedRules(object, &exception);
    if (exception.shouldThrow()) {
        throw exception;
    }
}

ConfigFile *ConfigurationParser::_loadConfigFile(const std::string &filePath) {
    std::fstream file(filePath);
    if (!file.is_open())
        throw ParserException("Failed to open configuration file: " + filePath);

    auto it = _configFiles.emplace(filePath, _arena.alloc<ConfigFile>(filePath, std::string(), std::vector<Token*>(), std::vector<size_t>()));
    if (!it.second)
        throw ParserException("Circulair import detected for: " + filePath);
    ConfigFile *configFile = it.first->second;

    std::string line;
    while (std::getline(file, line)) {
        configFile->lineStarts.push_back(configFile->fileContent.size());
        configFile->fileContent.insert(configFile->fileContent.end(), line.begin(), line.end());
        configFile->fileContent.push_back('\n');
    }

    if (!configFile->fileContent.empty())
        configFile->fileContent.pop_back();
    configFile->fileContent.push_back('\0');
    file.close();

    _tokenize(configFile);
    _objects[filePath] = _getObjectFromFile(configFile);
    return (configFile);
}

bool ConfigurationParser::parseFile(const std::string &filePath) {
    if (!isFileLoaded(filePath)) {
        try {
            _loadConfigFile(filePath);
        } catch (const ParserException &e) {
            std::cerr << e.getMessage();
            return (false);
        } catch (const std::exception &e) {
            ERROR("Exception while parsing file: " + filePath + "\n" + e.what());
            return (false);
        }
    }

    return (true);
}

std::vector<ServerConfig> ConfigurationParser::getResult(const std::string &filePath) {
    auto it = _objects.find(filePath);
    if (it == _objects.end() || !it->second)
        return {};

    Object *result = it->second;
    DEBUG("Configuration object in " << filePath << ":\n" << *result);
    std::vector<ServerConfig> servers;
    ObjectParser objectParser(result);

    try {
        objectParser.local().required().parseRange(servers);
        checkUnusedRules(result);
    } catch (const ParserException &e) {
        std::cerr << e.getMessage();
        return {};
    } catch (const std::exception &e) {
        ERROR("Exception while processing configuration: " + std::string(e.what()));
        return {};
    }

    return servers;
}

void Object::printObject(std::ostream &os, int indentLevel) const {
    for (const auto &pair : rules)
        for (const auto &rule : pair.second)
            rule->printRule(os, indentLevel + 1);
}

void Rule::printRule(std::ostream &os, int indentLevel) const {
    ArgumentType lastType = ArgumentType::STRING;
    std::string indent(indentLevel * 2, ' ');
    os << indent << token->value;
    for (const auto &arg : arguments) {
        os << " ";
        lastType = arg->type;
        if (arg->type == ArgumentType::OBJECT) {
            os << "{\n";
            std::get<Object *>(arg->value)->printObject(os, indentLevel + 1);
            os << indent << "}";
        } else if (arg->type == ArgumentType::KEYWORD) {
            os << TERM_COLOR_MAGENTA << arg->token->value << TERM_COLOR_RESET;
        } else {
            os << "\"" << std::get<std::string>(arg->value) << "\"";
        }
    }
    if (lastType != ArgumentType::OBJECT)
        os << ";";
    if (!includeRuleRefs.empty()) {
        os << " " << TERM_COLOR_YELLOW << "#";
        for (const auto &includeRule : includeRuleRefs)
            os << " => " << includeRule->arguments[0]->token->value;
        os << TERM_COLOR_RESET;
    }
    os << "\n";
}

std::ostream& operator<<(std::ostream &os, const Object &object) {
    object.printObject(os, -1);
    return os;
}

std::ostream& operator<<(std::ostream &os, const Rule &rule) {
    rule.printRule(os);
    return os;
}