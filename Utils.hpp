#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

namespace Utils {
    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str, char delimiter);
    bool startsWith(const std::string& str, const std::string& prefix);
    int stringToInt(const std::string& str);
    size_t parseSize(const std::string& str);

    std::string getFileExtension(const std::string& path);
    std::string intToString(int value);
    std::string toLower(const std::string& str);
}

#endif // UTILS_HPP
