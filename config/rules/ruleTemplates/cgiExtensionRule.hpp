#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <vector>
#include <string>

class CgiExtensionRule : public BaseRule {
private:
    std::vector<std::string> _extensions;

public:
    constexpr static Key getKey() { return Key::CGI_EXTENSION; }
    constexpr static const std::string getRuleName() { return "cgi_extension"; }
    constexpr static const std::string getRuleFormat() { return CgiExtensionRule::getRuleName() + " <ext1> [<ext2> ...]"; }

    CgiExtensionRule(const CgiExtensionRule &other) = default;
    CgiExtensionRule& operator=(const CgiExtensionRule &other) = default;
    ~CgiExtensionRule() = default;
    
    CgiExtensionRule() = default;
    CgiExtensionRule(std::vector<Rule *> &rules);

    bool isSet() const;
    const std::vector<std::string>& getExtensions() const;
    bool isCGI(const Path &path) const;
};

std::ostream& operator<<(std::ostream &os, const CgiExtensionRule &rule);