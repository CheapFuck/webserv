#include "errorpageRule.hpp"
#include "../ruleParser.hpp"
#include "../rules.hpp"

#include <ostream>
#include <vector>
#include <map>

/// @brief Add a single error page rule to the error pages map, overwriting any existing entry for the same error code.
/// @param rule The rule to be added, which should contain at least one error code and a path to the error page.
void ErrorPageRule::_addSingleRule(Rule *rule) {
    std::vector<StatusCode> errorCodes;
    Path errorPagePath;

    RuleParser::create(rule, *this)
        .expectMinNumArguments(2)
        .parseAllButX(errorCodes, 1)
        .parseArgument(errorPagePath);

    for (StatusCode code : errorCodes)
        _errorPages[code] = errorPagePath;
}

ErrorPageRule::ErrorPageRule(std::vector<Rule*> &rules) {
    for (Rule *rule : rules)
        _addSingleRule(rule);
}

/// @brief Check if the error page rule is set (i.e., if it contains any error pages).
bool ErrorPageRule::isSet() const {
    return (!_errorPages.empty());
}

/// @brief Get the map of error pages, where keys are error codes and values are paths to the corresponding error pages.
const std::map<StatusCode, Path>& ErrorPageRule::getErrorPages() const {
    return _errorPages;
}

/// @brief Get the error page path for a specific error code.
/// @param code The error code for which the error page is requested.
/// @return The path to the error page associated with the given error code, or an empty string if the code is not found.
/// If the code is StatusCode::Wildcard(), it returns the first error page found in the map.
std::string ErrorPageRule::getErrorPage(StatusCode code) const {
    auto it = _errorPages.find(code);
    if (it != _errorPages.end())
        return it->second.str();

    auto wildcardIt = _errorPages.find(StatusCode::Wildcard());
    if (wildcardIt != _errorPages.end())
        return wildcardIt->second.str();

    return "";
}

std::ostream& operator<<(std::ostream &os, const ErrorPageRule &rule) {
    os << "ErrorPageRule: ";
    for (const auto &pair : rule.getErrorPages()) {
        os << pair.first.value << " -> " << pair.second.str() << ", ";
    }

    if (!rule.isSet())
        os << "No error pages set.";

    return os;
}
