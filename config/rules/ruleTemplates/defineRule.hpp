#pragma once

#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <vector>
#include <string>
#include <map>

class DefineRule : public BaseRule {
private:
    std::string _name;
    Object *_object = nullptr;

public:
    constexpr static Key getKey() { return Key::DEFINE; }
    constexpr static const std::string getRuleName() { return "define"; }
    constexpr static const std::string getRuleFormat() { return DefineRule::getRuleName() + " <name> <object>"; }

    DefineRule(const DefineRule &other) = default;
    DefineRule& operator=(const DefineRule &other) = default;
    ~DefineRule() = default;
    
    DefineRule() = delete;
    DefineRule(Rule *rule);

    const std::string& getName() const;
    Object* getObject() const;
    bool isSet() const;
};

std::ostream& operator<<(std::ostream &os, const DefineRule &rule);