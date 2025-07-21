#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <vector>
#include <string>

class IndexRule : public BaseRule {
private:
    std::vector<std::string> _indexFiles;

public:
    constexpr static Key getKey() { return Key::INDEX; }
    constexpr static const char* getRuleName() { return "index"; }
    constexpr static const char* getRuleFormat() { return "index <file1> [<file2> ...]"; }

    IndexRule(const IndexRule &other) = default;
    IndexRule& operator=(const IndexRule &other) = default;
    ~IndexRule() = default;
    
    IndexRule() = default;
    IndexRule(std::vector<Rule *> &rules);

    bool isSet() const;
    const std::vector<std::string>& getIndexFiles() const;
};

std::ostream& operator<<(std::ostream &os, const IndexRule &rule);