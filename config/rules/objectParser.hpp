#pragma once

#include "../parserExceptions.hpp"

#include <string>

enum RulesScope {
    LOCAL = 0,
    BOUND = 1,
    GLOBAL = 2,
};

enum ExpectedRuleCount {
    ONE = 1,
    MULTIPLE = 2,
};

class ObjectParser {
private:
    ExpectedRuleCount _expectedRuleCount = ExpectedRuleCount::ONE;
    RulesScope _scopeFallback = RulesScope::LOCAL;
    Key _bound_fallback = Key::NO_KEY;
    bool _optional = false;
    Object *_object;

    std::vector<Rule*> _fetchRules(Key key);

public:
    ObjectParser(Object *object);
    ObjectParser(const ObjectParser&) = default;
    ObjectParser& operator=(const ObjectParser&) = default;
    ~ObjectParser() = default;

    ObjectParser& local();
    ObjectParser& bound(Key key);
    ObjectParser& global();
    ObjectParser& optional();
    ObjectParser& required();

    /// @brief Parse a single target class from a single rule.
    /// @tparam T The target class type that will be parsed from the rule.
    /// @param target The target class instance that will be filled with the parsed rule data.
    /// @return A reference to the current ObjectParser instance for method chaining.
    template <typename T>
    ObjectParser& parseFromOne(T& target) {
        _expectedRuleCount = ExpectedRuleCount::ONE;

        Key key = T::getKey();
        std::vector<Rule*> rules;
        try { rules = _fetchRules(key); }
        catch (ParserMissingException &e) {
            e.attachObject(_object);
            throw;
        }

        if (rules.empty()) target = std::move(T(nullptr));
        else target = std::move(T(rules[0]));

        return (*this);
    }

    /// @brief Parse a target class from multiple rules.
    /// @tparam T The target class type that will be parsed from the rules.
    /// @param target The target class instance that will be filled with the parsed rule data.
    /// @return A reference to the current ObjectParser instance for method chaining.
    template <typename T>
    ObjectParser& parseFromRange(T &target) {
        _expectedRuleCount = ExpectedRuleCount::MULTIPLE;

        Key key = T::getKey();
        std::vector<Rule*> rules;
        try { rules = _fetchRules(key); }
        catch (ParserMissingException &e) {
            e.attachObject(_object);
            throw;
        }

        target = std::move(T(rules));

        return (*this);
    }

    /// @brief Parse a range of target classes from multiple rules.
    /// @tparam T The target class type that will be parsed from the rules.
    /// @param target The target vector that will be filled with the parsed rule data.
    /// @return A reference to the current ObjectParser instance for method chaining.
    template <typename T>
    ObjectParser& parseRange(std::vector<T>& target) {
        _expectedRuleCount = ExpectedRuleCount::MULTIPLE;

        Key key = T::getKey();
        std::vector<Rule*> rules;
        try { rules = _fetchRules(key); }
        catch (ParserMissingException &e) {
            e.attachObject(_object);
            throw;
        }

        for (Rule* rule : rules)
            target.emplace_back(T(rule));

        return (*this);
    }
};
