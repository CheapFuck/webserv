#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <string>

class ReturnRule : public BaseRule {
private:
    bool _isSet;
    StatusCode _statusCode;
    std::string _parameter;

public:
    constexpr static Key getKey() { return Key::RETURN; }
    constexpr static const char* getRuleName() { return "return"; }
    constexpr static const char* getRuleFormat() { return "return <status_code> [<parameter>]"; }

    ReturnRule(const ReturnRule &other) = default;
    ReturnRule& operator=(const ReturnRule &other) = default;
    ~ReturnRule() = default;

    ReturnRule();
    ReturnRule(Rule *rule);

    bool isSet() const;
    bool isRedirect() const;
    const StatusCode getStatusCode() const;
    const std::string& getParameter() const;
};

std::ostream& operator<<(std::ostream &os, const ReturnRule &rule);
