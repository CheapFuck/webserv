#include "../ruleParser.hpp"
#include "includeRule.hpp"
#include "../rules.hpp"

#include <ostream>

IncludeRule::IncludeRule(Rule *rule) {
    RuleParser::create(rule, *this)
        .expectArgumentCount(1)
        .parseArgument(_includePath);
}

/// @brief Get the include path specified in the rule.
const std::string& IncludeRule::getIncludePath() const {
    return _includePath;
}

/// @brief Check if the include rule is set (i.e., if it has a non-empty include path).
bool IncludeRule::isSet() const {
    return (!_includePath.empty());
}

std::ostream& operator<<(std::ostream &os, const IncludeRule &rule) {
    os << "IncludeRule: " << rule.getIncludePath();
    return os;
}
