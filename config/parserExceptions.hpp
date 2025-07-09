#pragma once

#include "config.hpp"

#include <string>
#include <vector>

#define DUPLICATE_RULE

class ParserException : public std::exception {
protected:
    std::string _message;
    std::string _hint;
    std::vector<ErrorContext> _traceback;

    void _printErrorContext(const ErrorContext &context, const Token *token, std::ostream &os) const;
    void _printCompactErrorContext(const ErrorContext &context, const Token *token, std::ostream &os) const;
    void _printHint(std::ostream &os) const;
    void _printTraceback(std::ostream &os, const std::vector<ErrorContext> *traceback = nullptr) const;

public:
    ParserException(const std::string &message, const std::string &hint = "");
    ParserException(const std::string &message, const std::string &hint, std::vector<ErrorContext> traceback);
    virtual ~ParserException() noexcept = default;

    void addTracebackFromRule(Rule *rule);
    virtual std::string getMessage() const;
};

class ParserTokenException : public ParserException {
private:
    Token *_token;

public:
    ParserTokenException(const std::string &message, Token *token, const std::string &hint = "");

    std::string getMessage() const override;

};

class ParserRuleException : public ParserException {
private:
    const Rule *_rule;

public:
    ParserRuleException(const std::string &message, const Rule *rule, const std::string &hint = "");

    std::string getMessage() const override;
};

class ParserArgumentException : public ParserException {
private:
    const Argument *_argument;

public:
    ParserArgumentException(const std::string &message, const Argument *argument, const std::string &hint = "");

    std::string getMessage() const override;
};

class ParserDuplicateRuleException : public ParserException {
private:
    const Rule *_firstRule;
    const Rule *_secondRule;
    std::vector<ErrorContext> _tracebackFirst;

public:
    ParserDuplicateRuleException(const std::string &message, const Rule *firstRule, const Rule *secondRule, const std::string &hint = "");

    std::string getMessage() const override;
};

class ParserMissingException : public ParserException {
private:
    const Object *_object;

public:
    ParserMissingException(const std::string &message, const std::string &hint = "");

    void attachObject(const Object *object);
    std::string getMessage() const override;
};