#include "../ruleParser.hpp"
#include "indexRule.hpp"
#include "../rules.hpp"

#include <ostream>
#include <vector>

IndexRule::IndexRule(std::vector<Rule *> &rules) {
    for (Rule *rule : rules) {
        RuleParser::create(rule, *this)
            .expectMinNumArguments(1)
            .parseAll(_indexFiles);
    }
}

/// @brief Check if the index rule is set (i.e., if it has any index files defined).
bool IndexRule::isSet() const {
    return (!_indexFiles.empty());
}

/// @brief Get the list of index files defined by this rule.
const std::vector<std::string>& IndexRule::getIndexFiles() const {
    return _indexFiles;
}

std::ostream& operator<<(std::ostream &os, const IndexRule &rule) {
    os << "IndexRule: ";
    if (rule.isSet()) {
        os << "Index files: ";
        for (const auto &file : rule.getIndexFiles()) {
            os << file << " ";
        }
    } else {
        os << "No index files set";
    }
    return os;
}
