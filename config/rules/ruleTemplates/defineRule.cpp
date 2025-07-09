#include "../ruleParser.hpp"
#include "defineRule.hpp"
#include "../rules.hpp"

DefineRule::DefineRule(Rule *rule) {
    RuleParser::create(rule, *this)
        .expectArgumentCount(2)
        .parseArgument(_name)
        .parseArgument(_object);
}

/// @brief Get the name of the defined object.
const std::string& DefineRule::getName() const {
    return _name;
}

/// @brief Get the object that is defined by this rule.
Object* DefineRule::getObject() const {
    return _object;
}

/// @brief Check if the rule is set (i.e., if it has a name and an object).
bool DefineRule::isSet() const {
    return (!_name.empty() && _object != nullptr);
}

std::ostream& operator<<(std::ostream &os, const DefineRule &rule) {
    os << "DefineRule: " << rule.getName() << " -> " << (rule.getObject() ? "{ Some Object }" : "{ No Object }");
    return os;
}
