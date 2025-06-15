#include "../../print.hpp"
#include "../config.hpp"
#include "rules.hpp"

#include <unordered_map>
#include <functional>

/// @brief Extracts rules from the given object based on the provided rule parsers.
/// @param object The object containing rules to be parsed.
/// @param ruleParsers A map of keys to functions that parse the corresponding rules.
/// @param throwOnNonEmptyReturn If true, throws an exception if there are any unexpected keys left in the object after parsing.
/// @throws ConfigParsingException if an unexpected key is found in the object or if there are any remaining keys after parsing.
void extractRules(Object &object, const std::unordered_map<Key, std::function<void(Rules &)>> &ruleParsers, bool throwOnNonEmptyReturn) {
    for (auto &[key, rules] : ruleParsers) {
        auto it = object.find(key);
        if (it != object.end()) {
            rules(it->second);
            object.erase(it);
        }
    }

    if (!object.empty() && throwOnNonEmptyReturn) {
        std::string unexpected_keys;
        for (const auto &rule : object) {
            unexpected_keys += std::to_string(rule.first) + " ";
        }
        throw ConfigParsingException("Unexpected keys in server configuration: " + unexpected_keys);
    }
}
