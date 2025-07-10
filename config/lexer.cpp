#include "parserExceptions.hpp"
#include "../print.hpp"
#include "config.hpp"

#include <ostream>
#include <cstring>
#include <string>

static const TokenPattern tokenPatterns[] = {
    {"{", 1, TokenType::OBJECT_OPEN},
    {"}", 1, TokenType::OBJECT_CLOSE},
    {";", 1, TokenType::RULE_END},
    {"#", 1, TokenType::COMMENT},
    {"'", 1, TokenType::QUOTE1},
    {" ", 1, TokenType::WHITESPACE},
    {"\"", 1, TokenType::QUOTE2},
    {"\n", 1, TokenType::LINE_END},
    {"\t", 1, TokenType::WHITESPACE},
    {"\r", 1, TokenType::WHITESPACE},
    {"\v", 1, TokenType::WHITESPACE},
    {"\f", 1, TokenType::WHITESPACE},
};

Token *ConfigurationParser::getNextToken(ConfigFile *configFile, size_t &pos) {
    for (const TokenPattern &pattern : tokenPatterns) {
        if (configFile->fileContent.compare(pos, pattern.length, pattern.pattern) == 0) {
            pos += pattern.length;
            return (_arena.alloc<Token>(
                pattern.type,
                pattern.pattern,
                configFile,
                pos - pattern.length
            ));
        }
    }

    pos++;
    return (_arena.alloc<Token>(
        configFile->fileContent[pos - 1] ? TokenType::WEAK_STR : TokenType::END,
        std::string(1, configFile->fileContent[pos - 1]),
        configFile,
        pos - 1
    ));
}

Token *ConfigurationParser::parseContinuousToken(ConfigFile *configFile, size_t &pos, Token *currentToken, TokenType endOfStringTypeMask) {
    while (true) {
        Token *nextToken = getNextToken(configFile, pos);
        if (nextToken->type & endOfStringTypeMask)
            return (nextToken);
        else
            currentToken->value += nextToken->value;
    }
}

void ConfigurationParser::_tokenize(ConfigFile *configFile) {
    size_t pos = 0;

    Token *previousToken;
    Token *currentToken = _arena.alloc<Token>(TokenType::OBJECT_OPEN, "", configFile, 0);
    do {
        previousToken = currentToken;

        switch (previousToken->type) {
            case TokenType::END:
                configFile->tokens.push_back(_arena.alloc<Token>(TokenType::OBJECT_CLOSE, "", configFile, pos));
                break ;

            case TokenType::WEAK_STR:
                currentToken = parseContinuousToken(configFile, pos, previousToken, EOS_MASK_DEFAULT);
                break ;

            case TokenType::COMMENT:
                previousToken->value.clear();
                currentToken = parseContinuousToken(configFile, pos, previousToken, COMMENT_MASK);
                break ;

            case TokenType::QUOTE1:
            case TokenType::QUOTE2:
                currentToken = getNextToken(configFile, pos);
                if (currentToken->type == previousToken->type)
                    throw ParserTokenException("Quote without content in configuration file", previousToken, "Put some content in the quotes; or remove them if not needed!");
                if (parseContinuousToken(configFile, pos, currentToken, EOS_MASK_QUOTE(previousToken->type))->type != previousToken->type)
                    throw ParserTokenException("Unmatched quote in configuration file", previousToken, "Close it dummy!");
                currentToken->type = TokenType::STR; // Promote to STR type
                previousToken = currentToken;
                [[fallthrough]];

            default:
                currentToken = getNextToken(configFile, pos);
        }

        if (IS_USABLE_TOKEN_TYPE(previousToken->type))
            configFile->tokens.push_back(previousToken);

    } while (previousToken->type != TokenType::END);
}

std::ostream &operator<<(std::ostream &os, const Token &token) {
    return os << "Token(type=" << token.type << ", value=\"" << token.value << "\", filePos=" << token.filePos << ")";
}