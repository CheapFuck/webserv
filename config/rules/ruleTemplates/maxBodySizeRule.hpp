#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <string>

class MaxBodySizeRule : public BaseRule {
private:
    bool _isSet;
    Size _maxBodySize;

public:
    constexpr static Key getKey() { return Key::MAX_BODY_SIZE; }
    constexpr static const std::string getRuleName() { return "client_max_body_size"; }
    constexpr static const std::string getRuleFormat() { return MaxBodySizeRule::getRuleName() + " <size>"; }

    MaxBodySizeRule(const MaxBodySizeRule &other) = default;
    MaxBodySizeRule& operator=(const MaxBodySizeRule &other) = default;
    ~MaxBodySizeRule() = default;

    MaxBodySizeRule();
    MaxBodySizeRule(Rule *rule);

    bool isSet() const;
    const Size& getMaxBodySize() const;
};

std::ostream& operator<<(std::ostream &os, const MaxBodySizeRule &rule);
