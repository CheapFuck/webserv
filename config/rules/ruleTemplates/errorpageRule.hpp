#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <vector>
#include <string>
#include <map>

class ErrorPageRule : public BaseRule {
private:
    std::map<StatusCode, Path> _errorPages;

    void _addSingleRule(Rule *rule);

public:
    constexpr static Key getKey() { return Key::ERROR_PAGE; }
    constexpr static const std::string getRuleName() { return "error_page"; }
    constexpr static const std::string getRuleFormat() { return ErrorPageRule::getRuleName() + " <status_code> <path>"; }

    ErrorPageRule(const ErrorPageRule &other) = default;
    ErrorPageRule& operator=(const ErrorPageRule &other) = default;
    ~ErrorPageRule() = default;
    
    ErrorPageRule() = default;
    ErrorPageRule(std::vector<Rule*> &rules);

    std::string getErrorPage(StatusCode code) const;

    bool isSet() const;
    const std::map<StatusCode, Path>& getErrorPages() const;
};

std::ostream& operator<<(std::ostream &os, const ErrorPageRule &rule);