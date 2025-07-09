#pragma once

#include "../types/argumentConverter.hpp"
#include "../parserExceptions.hpp"
#include "baserule.hpp"

#include <string>

class BaseRule;

class RuleParser {
private:
    Rule* _rule;
    std::string _ruleName;
    std::string _ruleFormat;

    size_t _argumentIndex = 0;

public:
    RuleParser(Rule* rule, const std::string &name, const std::string &format);
    ~RuleParser() = default;

    template <typename T, typename = std::enable_if_t<std::is_base_of<BaseRule, T>::value>>
    static RuleParser create(Rule *rule, T &context) {
        return RuleParser(rule, context.getRuleName(), context.getRuleFormat());
    }

    RuleParser& expectMinNumArguments(size_t min);
    RuleParser& expectArgumentCount(size_t min, size_t max);
    RuleParser& expectArgumentCount(size_t count);

    template <typename T>
    RuleParser& parseArgument(T &target) {
        if (_argumentIndex >= _rule->arguments.size())
            throw ParserRuleException("Not enough arguments provided for rule '" + _ruleName + "'", _rule,
                "Check the syntax of the rule. Expected format:\n\t" + _ruleFormat);

        target = std::move(ArgumentConverter<T, Argument*>::convert(_rule->arguments[_argumentIndex++]));
        return (*this);
    }

    template <typename T>
    RuleParser& parseOptionalArgument(T &target) {
        if (_argumentIndex < _rule->arguments.size())
            target = std::move(ArgumentConverter<T, Argument*>::convert(_rule->arguments[_argumentIndex++]));

        return (*this);
    }

    template <typename T>
    RuleParser& parseAllButX(std::vector<T> &target, size_t argumentsToSave) {
        if (argumentsToSave >= _rule->arguments.size() - _argumentIndex)
            throw ParserRuleException("Not enough arguments provided for rule '" + _ruleName + "'", _rule,
                "Check the syntax of the rule. Expected format:\n\t" + _ruleFormat);
        size_t argumentsToParse = _rule->arguments.size() - _argumentIndex - argumentsToSave;

        for (size_t i = 0; i < argumentsToParse; ++i)
            target.emplace_back(ArgumentConverter<T, Argument*>::convert(_rule->arguments[_argumentIndex++]));
        return (*this);
    }

    template <typename T>
    RuleParser& parseAll(std::vector<T> &target) {
        return parseAllButX<T>(target, 0);
    }
};

