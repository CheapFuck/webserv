#include "config/rules/ruleParser.hpp"
#include "config/rules/ruleTemplates/returnRule.hpp"
#include "config/rules/rules.hpp"

#include <ostream>

ReturnRule::ReturnRule() :
    _isSet(false), _statusCode(StatusCode::Wildcard()), _parameter("") {}

ReturnRule::ReturnRule(Rule *rule) :
    _isSet(false), _statusCode(StatusCode::Wildcard()), _parameter("") {
    if (!rule) return;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1, 2)
        .parseArgument(_statusCode)
        .parseOptionalArgument(_parameter);

    _isSet = true;
}

/// @brief Check if the return rule is set (i.e., if it has a status code and an optional parameter).
bool ReturnRule::isSet() const {
    return _isSet;
}

/// @brief Check if the return rule is a redirect (i.e., if it has a status code in the 3xx range).
bool ReturnRule::isRedirect() const {
    return (isSet() && _statusCode >= 300 && _statusCode < 400);
}

/// @brief Get the status code specified in the return rule.
const StatusCode ReturnRule::getStatusCode() const {
    return _statusCode;
}

/// @brief Get the optional parameter specified in the return rule.
const std::string& ReturnRule::getParameter() const {
    return _parameter;
}

std::ostream& operator<<(std::ostream &os, const ReturnRule &rule) {
    os << "ReturnRule: ";
    if (rule.isSet()) {
        os << "Status Code: " << rule.getStatusCode().value;
        if (!rule.getParameter().empty())
            os << ", Parameter: " << rule.getParameter();
    } else {
        os << "Not set";
    }
    return os;
}
