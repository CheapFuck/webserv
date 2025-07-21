#include "config/rules/ruleParser.hpp"
#include "config/rules/ruleTemplates/aliasRule.hpp"
#include "config/rules/rules.hpp"

#include <ostream>

AliasRule::AliasRule() : _aliasPath(Path::createDummy()) {}

AliasRule::AliasRule(Rule *rule) : _aliasPath(Path::createDummy()) {
    if (!rule) return;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1)
        .parseArgument(_aliasPath);
}

/// @brief Check if the alias rule is set (i.e., if it has been parsed and contains a valid path).
bool AliasRule::isSet() const {
    return (!_aliasPath.str().empty());
}

/// @brief Get the alias path defined by this rule.
const Path &AliasRule::getAliasPath() const {
    return _aliasPath;
}

std::ostream &operator<<(std::ostream &os, const AliasRule &rule) {
    os << "AliasRule: ";
    if (rule.isSet()) {
        os << "Alias Path: " << rule.getAliasPath().str();
    } else {
        os << "No alias path set";
    }
    return os;
}
