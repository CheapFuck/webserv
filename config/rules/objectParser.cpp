#include "objectParser.hpp"

#include <vector>

ObjectParser::ObjectParser(Object *object)
    : _object(object) {}

/// @brief Fetch rules for a given key from the current object and its parent objects.
/// The rules are fetched in a depth-first manner, starting from the current object and going up
/// through its parent rules - taking into account the specified scope
/// @param key The key for which the rules are being fetched.
/// @return A vector of pointers to the rules that match the given key. The rules are ordered from the least specific (global scope) to the most specific (local scope).
std::vector<Rule*> ObjectParser::_fetchRules(Key key) {
    std::vector<Rule*> rules;
    Object *object = _object;

    while (object) {
        auto it = object->rules.find(key);
        if (it != object->rules.end())
            rules.insert(rules.begin(), it->second.begin(), it->second.end());

        if (!object->parentRule
            || (_expectedRuleCount == ExpectedRuleCount::ONE && rules.size())
            || (_scopeFallback == RulesScope::BOUND && _bound_fallback == object->parentRule->key)
            || _scopeFallback == RulesScope::LOCAL)
            break ;

        object = object->parentRule->parentObject;
    }

    if (rules.empty() && !_optional)
        throw ParserMissingException("Missing rule for key: " + std::to_string(static_cast<int>(key)), 
            "Check the configuration file for the required rule.");
    if (_expectedRuleCount == ExpectedRuleCount::ONE && rules.size() > 1)
        throw ParserDuplicateRuleException("Duplicated rule found", rules[0], rules[1],
            "Remove the duplicate rule outside of its scope to avoid conflicts.");

    return (rules);
}

/// @brief Set the scope to local -> the rules will be searched in the current object only.
/// @return A reference to the current ObjectParser instance for method chaining.
ObjectParser& ObjectParser::local() {
    _scopeFallback = RulesScope::LOCAL;
    return (*this);
}

/// @brief Set the scope to bound -> the rules will be searched in the current object and its parent objects until the parent rule has the specified key.
/// @param key The key of the parent rule that will bound the search.
/// @return A reference to the current ObjectParser instance for method chaining.
ObjectParser& ObjectParser::bound(Key key) {
    _scopeFallback = RulesScope::BOUND;
    _bound_fallback = key;
    return (*this);
}

/// @brief Set the scope to global -> the rules will be searched in all objects (even those outside of the server block!).
/// @return A reference to the current ObjectParser instance for method chaining.
ObjectParser& ObjectParser::global() {
    _scopeFallback = RulesScope::GLOBAL;
    return (*this);
}

/// @brief Set the parser to optional -> the parser will not throw an exception if no rules are found.
/// @return A reference to the current ObjectParser instance for method chaining.
ObjectParser& ObjectParser::optional() {
    _optional = true;
    return (*this);
}

/// @brief Set the parser to required -> the parser will throw an exception if no rules are found.
/// @return A reference to the current ObjectParser instance for method chaining.
ObjectParser& ObjectParser::required() {
    _optional = false;
    return (*this);
}
