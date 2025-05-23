#include "Utils.hpp"
#include <algorithm>
#include <sstream>
#include <cctype>

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

    bool startsWith(const std::string& str, const std::string& prefix) {
        return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
    }

    int stringToInt(const std::string& str) {
        std::istringstream iss(str);
        int value;
        iss >> value;
        return value;
    }

    size_t parseSize(const std::string& str) {
        std::string sizeStr = str;
        size_t multiplier = 1;

        if (sizeStr.empty()) {
            return 0;
        }

        char unit = sizeStr[sizeStr.length() - 1];
        if (!std::isdigit(unit)) {
            sizeStr = sizeStr.substr(0, sizeStr.length() - 1);
            
            switch (unit) {
                case 'k':
                case 'K':
                    multiplier = 1024;
                    break;
                case 'm':
                case 'M':
                    multiplier = 1024 * 1024;
                    break;
                case 'g':
                case 'G':
                    multiplier = 1024 * 1024 * 1024;
                    break;
                default:
                    return 0;
            }
        }
        
        return stringToInt(sizeStr) * multiplier;
    }
}
