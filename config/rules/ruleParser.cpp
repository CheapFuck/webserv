#include "../parserExceptions.hpp"
#include "ruleParser.hpp"
#include "rules.hpp"

RuleParser::RuleParser(Rule *rule, const std::string &name, const std::string &format)
    : _rule(rule), _ruleName(name), _ruleFormat(format) {
    rule->isUsed = true;
}

/// @brief Check if the rule has at least a minimum number of arguments.
/// @param min The minimum number of arguments expected for the rule.
/// @return A reference to the current RuleParser instance for method chaining.
/// @throws ParserRuleException if the number of arguments is less than the minimum.
RuleParser& RuleParser::expectMinNumArguments(size_t min) {
    if (_rule->arguments.size() < min)
        throw ParserRuleException("Invalid number of arguments; found " + std::to_string(_rule->arguments.size()) + 
            ", expected at least " + std::to_string(min), _rule,
            "Check the syntax of the rule. Expected format:\n\t" + _ruleFormat);
    return (*this);
}

/// @brief Check if the rule has an argument count within a specified range.
/// @param min The minimum number of arguments expected.
/// @param max The maximum number of arguments expected.
/// @return A reference to the current RuleParser instance for method chaining.
/// @throws ParserRuleException if the number of arguments is outside the specified range.
RuleParser& RuleParser::expectArgumentCount(size_t min, size_t max) {
    if (_rule->arguments.size() < min || _rule->arguments.size() > max)
        throw ParserRuleException("Invalid number of arguments; found " + std::to_string(_rule->arguments.size()) + 
            ", expected between " + std::to_string(min) + " and " + std::to_string(max), _rule,
            "Check the syntax of the rule. Expected format:\n\t" + _ruleFormat);
    return (*this);
}

/// @brief Check if the rule has exactly a specified number of arguments.
/// @param count The exact number of arguments expected for the rule.
/// @return A reference to the current RuleParser instance for method chaining.
/// @throws ParserRuleException if the number of arguments does not match the expected count.
RuleParser& RuleParser::expectArgumentCount(size_t count) {
    if (_rule->arguments.size() != count)
        throw ParserRuleException("Invalid number of arguments; found " + std::to_string(_rule->arguments.size()) + 
            ", expected exactly " + std::to_string(count), _rule,
            "Check the syntax of the rule. Expected format:\n\t" + _ruleFormat);
    return (*this);
}