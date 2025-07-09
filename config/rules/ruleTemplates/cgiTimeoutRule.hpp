#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <string>

#define CGI_DEFAULT_TIMEOUT 30.0

class CgiTimeoutRule : public BaseRule {
private:
    bool _isSet = false;
    
public:
    Timespan timeout;

    constexpr static Key getKey() { return Key::CGI_TIMEOUT; }
    constexpr static const std::string getRuleName() { return "cgi_timeout"; }
    constexpr static const std::string getRuleFormat() { return CgiTimeoutRule::getRuleName() + " <timeout>"; }

    CgiTimeoutRule(const CgiTimeoutRule &other) = default;
    CgiTimeoutRule& operator=(const CgiTimeoutRule &other) = default;
    ~CgiTimeoutRule() = default;
    
    CgiTimeoutRule();
    CgiTimeoutRule(Rule *rule);

    bool isSet() const;
};

std::ostream& operator<<(std::ostream &os, const CgiTimeoutRule &rule);