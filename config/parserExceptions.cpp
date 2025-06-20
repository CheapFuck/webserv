#include "../print.hpp"
#include "config.hpp"

#include <sstream>

ParserTokenException::ParserTokenException(const std::string &message, const Rule &rule)
    : _message(message), _filePos(rule.filePos) {}

ParserTokenException::ParserTokenException(const std::string &message, const Token &token)
    : _message(message), _filePos(token.filePos) {}

ParserTokenException::ParserTokenException(const std::string &message, const Argument &argument)
    : _message(message), _filePos(argument.filePos) {}

std::string ParserTokenException::what(ConfigurationParser &parser) const noexcept {
    ErrorContext context = parser.getErrorContext(_filePos);
    std::ostringstream oss;
    oss << TERM_COLOR_RED << "[ParserTokenException]" << TERM_COLOR_RESET << ": " << _message << "\n";
    oss << "Error found in " << parser.getFilename() << " at line " << context.lineNum << ":" << context.columnNum << "\n";
    oss << context.line;
    oss << std::string(context.columnNum - 1, ' ') << TERM_COLOR_CYAN << "^" << TERM_COLOR_RESET << "\n";
    return oss.str();
}

ParserDuplicationException::ParserDuplicationException(const std::string &message, const Rule &firstRule, const Rule &secondRule)
    : _message(message.empty() ? "Duplicate rule found" : message), _firstFilePos(firstRule.filePos), _secondFilePos(secondRule.filePos) {}

std::string ParserDuplicationException::what(ConfigurationParser &parser) const noexcept {
    ErrorContext firstContext = parser.getErrorContext(_firstFilePos);
    ErrorContext secondContext = parser.getErrorContext(_secondFilePos);

    std::ostringstream oss;
    oss << TERM_COLOR_RED << "ParserDuplicationException" << TERM_COLOR_RESET << ": " << _message << "\n";
    oss << "First occurrence in " << parser.getFilename() << " at line " << firstContext.lineNum << ":" << firstContext.columnNum << "\n";
    oss << firstContext.line;
    oss << std::string(firstContext.columnNum - 1, ' ') << TERM_COLOR_CYAN << "^" << TERM_COLOR_RESET << "\n";
    oss << "Second occurrence in " << parser.getFilename() << " at line " << secondContext.lineNum << ":" << secondContext.columnNum << "\n";
    oss << secondContext.line;
    oss << std::string(secondContext.columnNum - 1, ' ') << TERM_COLOR_CYAN << "^" << TERM_COLOR_RESET << "\n";
    return oss.str();
}

ParserMissingException::ParserMissingException(const std::string &message)
    : _message(message), _firstFilePos(0), _secondFilePos(0) {}

std::string ParserMissingException::what(ConfigurationParser &parser) const noexcept {
    ErrorContext openingContext = parser.getErrorContext(_firstFilePos);
    ErrorContext closingContext = parser.getErrorContext(_secondFilePos);

    std::ostringstream oss;
    oss << TERM_COLOR_RED << "[ParserMissingException]" << TERM_COLOR_RESET << ": " << _message << "\n";
    oss << "Context created at " << parser.getFilename() << " at line " << openingContext.lineNum << ":" << openingContext.columnNum << "\n";
    oss << openingContext.line;
    oss << std::string(openingContext.columnNum - 1, ' ') << TERM_COLOR_CYAN << "^" << TERM_COLOR_RESET << "\n";
    oss << "Context closed at " << parser.getFilename() << " at line " << closingContext.lineNum << ":" << closingContext.columnNum << "\n";
    oss << closingContext.line;
    oss << std::string(closingContext.columnNum - 1, ' ') << TERM_COLOR_CYAN << "^" << TERM_COLOR_RESET << "\n";
    return oss.str();
}

void ParserMissingException::attachObject(const Object &object) {
    _firstFilePos = object.fileObjOpenPos;
    _secondFilePos = object.fileObjClosePos;
}
