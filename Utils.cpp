#include "Utils.hpp"

#include <algorithm>
#include <sstream>
#include <string>

namespace Utils {
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            return "";
        }

        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }

    std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::istringstream iss(str);
        std::string token;

        while (std::getline(iss, token, delimiter)) {
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }

        return tokens;
    }

    int stringToInt(const std::string& str) {
        std::istringstream iss(str);
        int value;
        iss >> value;
        return value;
    }

    std::string getFileExtension(const std::string& path) {
        size_t dotPos = path.find_last_of('.');
        if (dotPos != std::string::npos && dotPos < path.length() - 1) {
            return path.substr(dotPos);
        }
        return "";
    }

    std::string intToString(int value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

    std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
}
