#include "rules/rules.hpp"
#include "../print.hpp"
#include "config.hpp"

#include <fstream>

ConfigurationParser::ConfigurationParser(const std::string &filename) : _filename(filename), _configuration(), _lineNumbers(), _result() {}

/// @brief Reads the configuration file and stores its content in the _configuration string.
/// @throws std::runtime_error if the file cannot be opened.
void ConfigurationParser::_readFile() {
    std::ifstream file(_filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open configuration file: " + _filename);
    }

    std::string line;
    while (std::getline(file, line)) {
        _lineNumbers.push_back(_configuration.size());
        _configuration.insert(_configuration.end(), line.begin(), line.end());
        _configuration.push_back('\n');
    }

    _configuration.push_back('\0');
    file.close();
}

/// @brief Creates the server configurations.
/// @param object The root object returned by the parser containing the full (parsed) file content.
void ConfigurationParser::_fetchServerConfigs(Object &object) {
    std::vector<ServerConfig> serverConfigs = fetchServerConfigs(object);

    std::unordered_map<Key, std::function<void(Rules &)>> ruleParsers = {
		{SERVER, [&serverConfigs](Rules &rules) { 
			for (Rule & rule : rules) {
				if (rule.arguments.size() != 1)
					throw ParserTokenException("Invalid server rule format", rule);
                if (rule.arguments[0].type != OBJECT)
                    throw ParserTokenException("Invalid server rule argument type", rule.arguments[0]);
				serverConfigs.push_back(ServerConfig(rule.arguments[0].rules));
			}
		 }},
	};

	extractRules(object, ruleParsers, true);

    for (const ServerConfig &config : serverConfigs) {
        if (_result.find(config.port.get()) == _result.end())
            _result[config.port.get()] = std::vector<ServerConfig>();
        _result[config.port.get()].push_back(config);
    }
}

/// @brief Extracts the server configurations from the file and internally stores them.
/// @return A boolean indicating whether the configuration was successfully fetched or not.
bool ConfigurationParser::fetchConfiguration() {
    try {
        _readFile();
        std::vector<Token> tokens = _tokenize();
        Object parsedObject = _parseTokens(tokens);
        _fetchServerConfigs(parsedObject);
        return (true);
    } catch (const ParserTokenException &e) {
        std::cout << (e.describe(*this));
        return (false);
    } catch (const ParserDuplicationException &e) {
        std::cout << (e.describe(*this));
        return (false);
    } catch (const ParserMissingException &e) {
        std::cout << (e.describe(*this));
        return (false);
    } catch (const std::exception &e) {
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return (false);
    }
}

/// @brief Returns the parsed server configurations.
std::map<int, std::vector<ServerConfig>> ConfigurationParser::getResult() const {
    return _result;
}

/// @brief Returns the error context for a given position in the configuration file;
/// consisting of the line, column and full line content.
ErrorContext ConfigurationParser::getErrorContext(size_t pos) const {
    if (_lineNumbers.empty())
        return {0, 0, ""};
    
    if (pos >= _configuration.size())
        return {_lineNumbers.size(), 0, ""};

    size_t lineNum;
    for (lineNum = 1; lineNum < _lineNumbers.size(); ++lineNum) {
        if (_lineNumbers[lineNum] > pos)
            break ;
    }

    return {
        .lineNum = lineNum,
        .columnNum = pos - _lineNumbers[lineNum - 1] + (lineNum < _lineNumbers.size() ? 1 : 0),
        .line = _configuration.substr(_lineNumbers[lineNum - 1], (lineNum < _lineNumbers.size() ? _lineNumbers[lineNum] : _configuration.length()) - _lineNumbers[lineNum - 1])
    };
}
