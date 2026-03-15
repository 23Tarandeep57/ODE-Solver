#pragma once

#include <string>
#include <stdexcept>
#include <cctype>

enum class TokenType {
    NUMBER, SYMBOL,
    PLUS, MINUS, MUL, DIV, POW,
    LPAREN, RPAREN,
    EQUALS,
    END
};

struct Token {
    TokenType type;
    std::string value;
};

class Lexer {
    std::string src;
    size_t pos = 0;

    char peek() const { return pos < src.size() ? src[pos] : '\0'; }
    char get() { return src[pos++]; }

    void skip_ws() {
        while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos])))
            ++pos;
    }

public:
    explicit Lexer(std::string s) : src(std::move(s)) {}

    Token next_token() {
        skip_ws();
        if (pos >= src.size()) return {TokenType::END, ""};

        char c = peek();

        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            std::string num;
            while (pos < src.size() &&
                   (std::isdigit(static_cast<unsigned char>(src[pos])) || src[pos] == '.'))
                num += get();
            return {TokenType::NUMBER, num};
        }

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string name;
            while (pos < src.size() &&
                   (std::isalnum(static_cast<unsigned char>(src[pos])) || src[pos] == '_'))
                name += get();
            while (pos < src.size() && src[pos] == '\'')
                name += get();
            return {TokenType::SYMBOL, name};
        }

        get();
        switch (c) {
            case '+': return {TokenType::PLUS, "+"};
            case '-': return {TokenType::MINUS, "-"};
            case '*': return {TokenType::MUL,"*"};
            case '/': return {TokenType::DIV,"/"};
            case '^': return {TokenType::POW,"^"};
            case '(': return {TokenType::LPAREN,"("};
            case ')': return {TokenType::RPAREN, ")"};
            case '=': return {TokenType::EQUALS, "="};
            default:
                throw std::runtime_error(std::string("Unknown character: ") + c);
        }
    }
};
