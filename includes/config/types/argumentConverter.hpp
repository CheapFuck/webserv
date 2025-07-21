#pragma once

#include "../parserExceptions.hpp"
#include "config/types/customTypes.hpp"
#include "../../print.hpp"
#include "consts.hpp"

#include <limits>
#include <string>

template <typename To, typename From>
struct ArgumentConverter {
    static To convert(const From& from) {
        (void)from;
        throw std::runtime_error("Conversion not implemented for type " + std::string(typeid(To).name()) + " from type " + std::string(typeid(From).name()));
    }
};

template <>
struct ArgumentConverter<int, Argument*> {
    static int convert(const Argument* arg) {
        try {
            return (std::stoi(std::get<std::string>(arg->value)));
        } catch (...) {
            throw ParserArgumentException("Expected an integer", arg, \
                "Check the argument type. Expected an integer, but found: " + arg->token->value);
        }
    }
};

template <>
struct ArgumentConverter<PortNumber, Argument*> {
    static PortNumber convert(const Argument* arg) {
        try {
            return (PortNumber(std::stoi(std::get<std::string>(arg->value))));
        } catch (...) {
            throw ParserArgumentException("Expected an unsigned 16-bit integer", arg, \
                "Check the argument type. Expected an unsigned 16-bit integer, but found: " + arg->token->value);
        }
    }
};

template <>
struct ArgumentConverter<StatusCode, Argument*> {
    static StatusCode convert(const Argument* arg) {
        try {
            std::string value = std::get<std::string>(arg->value);
            if (value == "*")
                return StatusCode::Wildcard();
            return (StatusCode(std::stoi(value)));
        } catch (...) {
            throw ParserArgumentException("Expected a valid error code", arg, \
                "Check the argument type. Expected a valid error code {100 <= code <= 599}, but found: " + arg->token->value);
        }
    }
};

template <>
struct ArgumentConverter<std::string, Argument*> {
    static std::string convert(const Argument* arg) {
        if (arg->type != ArgumentType::STRING)
            throw ParserArgumentException("Expected a string", arg, \
                "Check the argument type. Expected a string, but found: " + arg->token->value);
        return std::get<std::string>(arg->value);
    }
};

template <>
struct ArgumentConverter<Object*, Argument*> {
    static Object* convert(const Argument* arg) {
        if (arg->type != ArgumentType::OBJECT)
            throw ParserArgumentException("Expected an object", arg, \
                "Check the argument type. Expected an object, but found: " + arg->token ->value);
        return std::get<Object*>(arg->value);
    }
};

template <>
struct ArgumentConverter<Size, Argument*> {
    static Size convert(const Argument* arg) {
        try {
            return Size(std::get<std::string>(arg->value));
        } catch (...) {
            throw ParserArgumentException("Expected a valid size", arg, \
                "Check the argument type. Expected a valid size {x (kb, mb, gb)}, but found: " + arg->token->value);
        }
    }
};

template <>
struct ArgumentConverter<Path, Argument*> {
    static Path convert(const Argument* arg) {
        if (arg->type != ArgumentType::STRING)
            throw ParserArgumentException("Expected a path", arg, \
                "Check the argument type. Expected a path, but found: " + arg->token->value);
        return Path(std::get<std::string>(arg->value));
    }
};

template <>
struct ArgumentConverter<bool, Argument*> {
    static bool convert(const Argument* arg) {
        if (arg->type != ArgumentType::KEYWORD)
            throw ParserArgumentException("Expected a boolean keyword", arg, \
                "Check the argument type. Expected a boolean keyword (on/off), but found: " + arg->token->value);
        switch (std::get<Keyword>(arg->value)) {
            case Keyword::ENABLE:
            case Keyword::TRUE:
            case Keyword::ON:
                return true;
            case Keyword::DISABLE:
            case Keyword::FALSE:
            case Keyword::OFF:
                return false;
            default:
                throw ParserArgumentException("Expected a boolean keyword", arg, \
                    "Check the argument type. Expected a boolean keyword (on/off), but found: " + arg->token->value);
        }
    }
};

template <>
struct ArgumentConverter<Timespan, Argument*> {
    static Timespan convert(const Argument* arg) {
        try {
            return Timespan(std::get<std::string>(arg->value));
        } catch (...) {
            throw ParserArgumentException("Expected a valid time span", arg, \
                "Check the argument type. Expected a valid time span in seconds, but found: " + arg->token->value);
        }
    }
};

template <>
struct ArgumentConverter<DefaultVal, Argument*> {
    static DefaultVal convert(const Argument* arg) {
        if (arg->type != ArgumentType::KEYWORD)
            throw ParserArgumentException("Expected a default value keyword", arg, \
                "Check the argument type. Expected a default value keyword (default), but found: " + arg->token->value);
        switch (std::get<Keyword>(arg->value)) {
            case Keyword::DEFAULT:
                return DefaultVal(true);
            default:
                throw ParserArgumentException("Expected a default value keyword", arg, \
                    "Check the argument type. Expected a default value keyword (default), but found: " + arg->token->value);
        }
    }
};

template <>
struct ArgumentConverter<Method, Argument*> {
    static Method convert(const Argument* arg) {
        if (arg->type != ArgumentType::STRING)
            throw ParserArgumentException("Expected a method keyword", arg, \
                "Check the argument type. Expected a method keyword (get/post/delete/put/head/options), but found: " + arg->token->value);
        std::string methodStr = std::get<std::string>(arg->value);
        Method method = stringToMethod(methodStr);
        if (method == UNKNOWN_METHOD)
            throw ParserArgumentException("Expected a method keyword", arg, \
                "Check the argument type. Expected a method keyword (get/post/delete/put/head/options), but found: " + arg->token->value);
        return (method);
    }
};
