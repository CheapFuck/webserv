#include "cgiExtensionRule.hpp"
#include "../ruleParser.hpp"
#include "../rules.hpp"

CgiExtensionRule::CgiExtensionRule(std::vector<Rule *> &rules) {
    for (Rule *rule : rules) {
        RuleParser::create(rule, *this)
            .expectMinNumArguments(1)
            .parseAll(_extensions);
    }

    for (std::string &ext : _extensions)
        if (!ext.empty() && *ext.begin() == '.')
            ext.erase(0, 1);
}

/// @brief Check if the CGI extension rule is set (i.e., if it contains any extensions).
bool CgiExtensionRule::isSet() const {
    return (!_extensions.empty());
}

/// @brief Get the list of CGI extensions defined by this rule.
const std::vector<std::string>& CgiExtensionRule::getExtensions() const {
    return (_extensions);
}

/// @brief Check if a given path has a CGI extension defined by this rule.
/// @param path The path to check.
/// @return True if the path has a CGI extension defined by this rule, false otherwise.
bool CgiExtensionRule::isCGI(const Path &path) const {
    if (_extensions.empty())
        return (false);

    std::string filename = path.getFilename();
    size_t lastDotPos = filename.find_last_of('.', filename.find_first_of('?'));
    if (lastDotPos == std::string::npos)
        return (false);

    std::string extension = filename.substr(lastDotPos + 1, filename.find_first_of('?') - lastDotPos - 1);
    return std::find(_extensions.begin(), _extensions.end(), extension) != _extensions.end();
}

std::ostream& operator<<(std::ostream &os, const CgiExtensionRule &rule) {
    os << "CgiExtensionRule: ";
    if (rule.isSet()) {
        os << "Extensions: ";
        for (const auto &ext : rule.getExtensions()) {
            os << ext << " ";
        }
    } else {
        os << "No extensions set";
    }
    return os;
}
