#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <string>

#define CGI_DEFAULT_ENABLED false

class CgiRule : public BaseRule {
private:
    bool _isSet;
    bool _isEnabled;

public:
    constexpr static Key getKey() { return Key::CGI_PASS; }
    constexpr static const char* getRuleName() { return "cgi"; }
    constexpr static const char* getRuleFormat() { return "cgi <path> [<timeout>]"; }

    CgiRule(const CgiRule &other) = default;
    CgiRule& operator=(const CgiRule &other) = default;
    ~CgiRule() = default;

    CgiRule();
    CgiRule(Rule *rule);

    bool isSet() const;
    bool isEnabled() const;
};

std::ostream& operator<<(std::ostream &os, const CgiRule &rule);