#pragma once

#include "lexer.h"
#include "expr.h"

class Parser {
    Lexer& lexer;
    Token cur_token;

    void advance() {
        cur_token = lexer.next_token();
    }

    int get_binding_pow(TokenType type) {
        switch (type) {
            case TokenType::PLUS:
            case TokenType::MINUS:
                return 10;
            case TokenType::MUL:
            case TokenType::DIV:
                return 20;
            case TokenType::POW:
                return 30;
            default:
                return 0;
        }
    }

public:
    explicit Parser(Lexer& l) : lexer(l) {
        advance();
    }

    ExprPtr parse_expression(int rbp = 0) {
        Token t = cur_token;
        advance();

        ExprPtr left = nud(t);

        while (rbp < get_binding_pow(cur_token.type)) {
            t = cur_token;
            advance();
            left = led(t, left);
        }
        return left;
    }

    ExprPtr nud(const Token& t) {
        switch (t.type) {
            case TokenType::NUMBER:
                return make_num(std::stod(t.value));

            case TokenType::SYMBOL: {
                // Check for known functions: ln(), exp()
                if (cur_token.type == TokenType::LPAREN &&
                    (t.value == "ln" || t.value == "exp"))
                {
                    advance(); // consume '('
                    ExprPtr arg = parse_expression();
                    if (cur_token.type != TokenType::RPAREN)
                        throw std::runtime_error("Expected ')' after function argument");
                    advance(); // consume ')'
                    if (t.value == "ln")  return make_log(arg);
                    if (t.value == "exp") return make_exp(arg);
                }
                return make_sym(t.value);
            }

            case TokenType::MINUS:
                // Unary minus: -expr =(-1) * expr
                return make_mul(make_num(-1), parse_expression(40));

            case TokenType::LPAREN: {
                ExprPtr expr = parse_expression();
                if (cur_token.type != TokenType::RPAREN)
                    throw std::runtime_error("Expected ')'");
                advance();
                return expr;
            }

            default:
                throw std::runtime_error("Unexpected token: " + t.value);
        }
    }

    ExprPtr led(const Token& t, ExprPtr left) {
        int bp = get_binding_pow(t.type);
        switch (t.type) {
            case TokenType::PLUS:
                return make_add(left, parse_expression(bp));
            case TokenType::MINUS:
                return make_sub(left, parse_expression(bp));
            case TokenType::MUL:
                return make_mul(left, parse_expression(bp));
            case TokenType::DIV:
                return make_div(left, parse_expression(bp));
            case TokenType::POW:
                return make_pow(left, parse_expression(bp - 1)); // right-associative
            default:
                throw std::runtime_error("Unexpected token in led: " + t.value);
        }
    }
};
