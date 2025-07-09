#include "parserExceptions.hpp"
#include "../print.hpp"
#include "config.hpp"

#include <sstream>
#include <string>

ErrorContext ConfigFile::getErrorContext(size_t pos) const {
    auto lineEndIt = std::lower_bound(lineStarts.begin(), lineStarts.end(), pos + 1);
    auto lineStartIt = (lineEndIt == lineStarts.begin()) ? lineStarts.begin() : std::prev(lineEndIt);

    size_t lineNumber = std::distance(lineStarts.begin(), lineEndIt);
    size_t columnNumber = pos - *lineStartIt;

    return (ErrorContext{
        .filename = fileName,
        .line = fileContent.substr(*lineStartIt, (lineEndIt != lineStarts.end() ? *lineEndIt : fileContent.size()) - *lineStartIt),
        .lineNumber = lineNumber,
        .columnNumber = columnNumber
    });
}

static std::vector<ErrorContext> loadTraceback(const Rule *rule) {
    std::vector<ErrorContext> traceback;

    const Rule *currentRule = rule;
    while (currentRule && currentRule->parentObject) {
        for (auto it = currentRule->includeRuleRefs.begin(); it != currentRule->includeRuleRefs.end(); ++it)
            traceback.push_back((*it)->token->configFile->getErrorContext((*it)->token->filePos));

        currentRule = currentRule->parentObject->parentRule;
    }

    return (traceback);
}

ParserException::ParserException(const std::string &message, const std::string &hint)
    : _message(message), _hint(hint), _traceback() {}

ParserException::ParserException(const std::string &message, const std::string &hint, std::vector<ErrorContext> traceback)
    : _message(message), _hint(hint), _traceback(std::move(traceback)) {}

void ParserException::_printErrorContext(const ErrorContext &context, const Token *token, std::ostream &oss) const {
    size_t errorLength = token->value.length();
    if (token->type & (TokenType::QUOTE1 | TokenType::QUOTE2))
        errorLength = 1;

    oss << "Error found in " << context.filename << " at line " << context.lineNumber << ":" << context.columnNumber + 1 << "\n";
    oss << context.line.substr(0, context.columnNumber) << TERM_COLOR_RED << TERM_BOLD << context.line.substr(context.columnNumber, errorLength) << TERM_COLOR_RESET << context.line.substr(context.columnNumber + errorLength);
    oss << std::string(context.columnNumber, ' ') << TERM_COLOR_CYAN << std::string(errorLength, '^') << TERM_COLOR_RESET << "\n";
}

void ParserException::_printCompactErrorContext(const ErrorContext &context, const Token *token, std::ostream &oss) const {
    size_t errorLength = token->value.length();
    if (token->type & (TokenType::QUOTE1 | TokenType::QUOTE2))
        errorLength = 1;

    oss << context.filename << ":" << context.lineNumber << ":" << context.columnNumber + 1 << ": ";
    oss << context.line.substr(0, context.columnNumber) << TERM_COLOR_RED << TERM_BOLD << context.line.substr(context.columnNumber, errorLength) << TERM_COLOR_RESET << context.line.substr(context.columnNumber + errorLength);
}

void ParserException::_printHint(std::ostream &os) const {
    if (!_hint.empty()) {
        os << TERM_COLOR_YELLOW << "Hint: " << _hint << TERM_COLOR_RESET << "\n\n";
    }
}

void ParserException::_printTraceback(std::ostream &os, const std::vector<ErrorContext> *traceback) const {
    if (!traceback) traceback = &_traceback;

    if (traceback->empty())
        return ;

    size_t maxFilenameLength = 0;
    for (const auto &context : *traceback)
        if (context.filename.length() > maxFilenameLength)
            maxFilenameLength = context.filename.length() + 1 + std::to_string(context.lineNumber).length();

    os << TERM_COLOR_RED << "Rule Traceback (inclusion chain):" << TERM_COLOR_RESET << "\n";
    for (const auto &context : *traceback) {
        size_t lineStart = context.line.find_first_not_of(" \t\r\n");
        if (lineStart == std::string::npos) continue ;

        std::string trimmedLine = context.line.substr(lineStart);
        if (!trimmedLine.back()) trimmedLine.pop_back();
        trimmedLine.erase(trimmedLine.find_last_not_of(" \t\r\n") + 1);

        size_t filenameLength = context.filename.length() + 1 + std::to_string(context.lineNumber).length();
        os << " → " << context.filename << ":" << context.lineNumber << std::string((maxFilenameLength - filenameLength + 3), ' ') << trimmedLine << "\n";
    }

    os << "\n";
}

void ParserException::addTracebackFromRule(Rule *rule) {
    std::vector<ErrorContext> newTraceback = loadTraceback(rule);
    _traceback.insert(_traceback.end(), rule->token->configFile->getErrorContext(rule->token->filePos));
    _traceback.insert(_traceback.end(), newTraceback.begin(), newTraceback.end());
}

std::string ParserException::getMessage() const {
    std::ostringstream oss;

    oss << TERM_COLOR_RED << "[ParserException]" << TERM_COLOR_RESET << ": " << _message << "\n\n";
    _printHint(oss);
    _printTraceback(oss);
    return (oss.str());
}

ParserTokenException::ParserTokenException(const std::string &message, Token *token, const std::string &hint)
    : ParserException(message, hint), _token(token) {}

ParserRuleException::ParserRuleException(const std::string &message, const Rule *rule, const std::string &hint)
    : ParserException(message, hint, loadTraceback(rule)), _rule(rule) {}

ParserArgumentException::ParserArgumentException(const std::string &message, const Argument *argument, const std::string &hint)
    : ParserException(message, hint, loadTraceback(argument->parentRule)), _argument(argument) {}

ParserDuplicateRuleException::ParserDuplicateRuleException(const std::string &message, const Rule *firstRule, const Rule *secondRule, const std::string &hint)
    : ParserException(message, hint, loadTraceback(secondRule)), _firstRule(firstRule), _secondRule(secondRule), _tracebackFirst(loadTraceback(firstRule)) {}

ParserMissingException::ParserMissingException(const std::string &message, const std::string &hint)
    : ParserException(message, hint), _object(nullptr) {}

std::string ParserTokenException::getMessage() const {
    ErrorContext context = _token->configFile->getErrorContext(_token->filePos);

    std::ostringstream oss;
    oss << TERM_COLOR_RED << "[ParserTokenException]" << TERM_COLOR_RESET << ": " << _message << "\n";
    _printErrorContext(context, _token, oss);
    _printHint(oss);
    _printTraceback(oss);
    return (oss.str());
}

std::string ParserRuleException::getMessage() const {
    ErrorContext context = _rule->token->configFile->getErrorContext(_rule->token->filePos);

    std::ostringstream oss;
    oss << TERM_COLOR_RED << "[ParserRuleException]" << TERM_COLOR_RESET << ": " << _message << "\n";
    _printErrorContext(context, _rule->token, oss);
    _printHint(oss);
    _printTraceback(oss);
    return (oss.str());
}

std::string ParserArgumentException::getMessage() const {
    ErrorContext context = _argument->token->configFile->getErrorContext(_argument->token->filePos);

    std::ostringstream oss;
    oss << TERM_COLOR_RED << "[ParserArgumentException]" << TERM_COLOR_RESET << ": " << _message << "\n";
    _printErrorContext(context, _argument->token, oss);
    _printHint(oss);
    _printTraceback(oss);
    return oss.str();
}

std::string ParserDuplicateRuleException::getMessage() const {
    ErrorContext contextFirstOccurance = _firstRule->token->configFile->getErrorContext(_firstRule->token->filePos);
    ErrorContext contextSecondOccurance = _secondRule->token->configFile->getErrorContext(_secondRule->token->filePos);

    std::ostringstream oss;
    oss << TERM_COLOR_RED << "[ParserDuplicateException]" << TERM_COLOR_RESET << ": " << _message << "\n";
    _printErrorContext(contextFirstOccurance, _firstRule->token, oss);
    oss << "Second occurrence:\n";
    _printErrorContext(contextSecondOccurance, _secondRule->token, oss);

    _printHint(oss);
    _printTraceback(oss, &_tracebackFirst);
    _printTraceback(oss, &_traceback);

    return oss.str();
}

void ParserMissingException::attachObject(const Object *object) {
    _traceback = loadTraceback(object->parentRule);
    _object = object;
}

std::string ParserMissingException::getMessage() const {
    if (!_object)
        throw ParserException("ParserMissingException: No object attached", "Attach an object to the exception before calling getMessage()");

    std::ostringstream oss;
    oss << TERM_COLOR_RED << "[ParserMissingException]" << TERM_COLOR_RESET << ": " << _message << "\n\n";

    ErrorContext openContext = _object->objectOpenToken->configFile->getErrorContext(_object->objectOpenToken->filePos);
    ErrorContext closeContext = _object->objectCloseToken->configFile->getErrorContext(_object->objectCloseToken->filePos);
    oss << "Object opened at:\n  > ";
    _printCompactErrorContext(openContext, _object->objectOpenToken, oss);
    oss << "Object closed at:\n  > ";
    _printCompactErrorContext(closeContext, _object->objectCloseToken, oss);
    oss << "\n";

    _printHint(oss);
    _printTraceback(oss);
    return oss.str();
}
