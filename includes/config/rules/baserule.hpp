#pragma once

#include "../config.hpp"

#include <string>

class BaseRule {
public:
    // These functions are meant to be overridden by child classes
    static Key getKey() { return Key::NO_KEY; }
    static const std::string& getRuleName() {
        static const std::string name = "base_rule";
        return name;
    }
    static const std::string& getRuleFormat() {
        static const std::string format = "base_rule <args>";
        return format;
    }

    bool isSet() const;
};
