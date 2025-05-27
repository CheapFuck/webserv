#pragma once

#include <string>
#include <vector>

namespace Utils {
    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str, char delimiter);
    int stringToInt(const std::string& str);
    std::string getFileExtension(const std::string& path);
    std::string intToString(int value);
    std::string toLower(const std::string& str);
}
