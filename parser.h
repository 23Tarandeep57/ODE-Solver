#pragma once

#include "expr.h"
#include "lexer.h"
#include <vector>

class Parser {
  std::vector<Token> tokens;
  size_t pos = 0;

  void advance() {
    if (pos < tokens.size())
      pos++;
  }

  Token cur() { return tokens[pos]; }

  void inject_implicit_mul(Lexer &lexer) {
    Token prev = {TokenType::END, ""};
    Token t = lexer.next_token();

    while (t.type != TokenType::END) {
      bool prev_is_value =
          (prev.type == TokenType::NUMBER || prev.type == TokenType::SYMBOL ||
           prev.type == TokenType::RPAREN);

      bool cur_starts_term =
          (t.type == TokenType::NUMBER || t.type == TokenType::SYMBOL ||
           t.type == TokenType::LPAREN);

      // Don't insert implicit mul before '(' if prev is a known function name
      bool prev_is_func = (prev.type == TokenType::SYMBOL &&
                           (prev.value == "ln" || prev.value == "exp"));

      if (prev_is_value && cur_starts_term && !prev_is_func)
        tokens.push_back({TokenType::MUL, "*"});
      tokens.push_back(t);
      prev = t;
      t = lexer.next_token();
    }

    tokens.push_back({TokenType::END, ""});
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
  explicit Parser(Lexer &lexer) { inject_implicit_mul(lexer); }

  ExprPtr parse_expression(int rbp = 0) {
    Token t = cur();
    advance();

    ExprPtr left = nud(t);

    while (rbp < get_binding_pow(cur().type)) {
      t = cur();
      advance();
      left = led(t, left);
    }
    return left;
  }

  ExprPtr nud(const Token &t) {
    switch (t.type) {
    case TokenType::NUMBER:
      return make_num(std::stod(t.value));

    case TokenType::SYMBOL: {
      // Check for known functions: ln(), exp()
      if (cur().type == TokenType::LPAREN &&
          (t.value == "ln" || t.value == "exp")) {
        advance(); // consume '('
        ExprPtr arg = parse_expression(0);
        if (cur().type != TokenType::RPAREN)
          throw std::runtime_error("Expected ')' after function argument");
        advance(); // consume ')'
        if (t.value == "ln")
          return make_log(arg);
        if (t.value == "exp")
          return make_exp(arg);
      }
      return make_sym(t.value);
    }

    case TokenType::MINUS:
      // Unary minus: -expr =(-1) * expr
      return make_mul(make_num(-1), parse_expression(40));

    case TokenType::LPAREN: {
      ExprPtr expr = parse_expression();
      if (cur().type != TokenType::RPAREN)
        throw std::runtime_error("Expected ')'");
      advance();
      return expr;
    }

    default:
      throw std::runtime_error("Unexpected NUD token: " + t.value);
    }
  }

  ExprPtr led(const Token &t, ExprPtr left) {
    int bp = get_binding_pow(t.type);
    switch (t.type) {
    case TokenType::PLUS:
      return make_add(left, parse_expression(bp));
    case TokenType::MINUS: // Subtraction: a - b → a + (-1 * b)
      return make_add(left, make_mul(make_num(-1), parse_expression(bp)));
    case TokenType::MUL:
      return make_mul(left, parse_expression(bp));
    case TokenType::DIV: // Division: a / b → a * b^(-1)
      return make_mul(left, make_pow(parse_expression(bp), make_num(-1)));
    case TokenType::POW:
      return make_pow(left, parse_expression(bp - 1)); // right-associative
    default:
      throw std::runtime_error("Unexpected token in led: " + t.value);
    }
  }
};
