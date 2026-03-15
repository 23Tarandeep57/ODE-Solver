#pragma once

#include "expr.h"
#include <stdexcept>
#include <string>
#include <vector>

class Integrator {
public:
  static ExprPtr integrate(const ExprPtr &e, const std::string &var) {
    if (!e)
      return nullptr;

    // 1. Base Case: Constant Integration -> int(c) dx = c * x
    if (auto *num = dynamic_cast<const Number *>(e.get())) {
      if (num->value == 0.0)
        return make_num(0);
      return make_mul(e, make_sym(var));
    }

    // 2. Base Case: Single Variable -> int(x) dx = x^2 / 2
    if (auto *sym = dynamic_cast<const Symbol *>(e.get())) {
      if (sym->name == var) {
        return make_mul(make_num(0.5), make_pow(e, make_num(2)));
      }
      // If variable is independent of the integration variable (e.g., int(y) dx
      // -> y*x)
      return make_mul(e, make_sym(var));
    }

    // 3. Composite Operations via N-ary Apply
    if (auto *app = dynamic_cast<const Apply *>(e.get())) {

      // A. Linearity: Sum Rule -> int(a + b + c) dx = int(a)dx + int(b)dx +
      // int(c)dx
      if (app->head == "Add") {
        std::vector<ExprPtr> integrated_args;
        integrated_args.reserve(app->args.size());
        for (const auto &arg : app->args) {
          integrated_args.push_back(integrate(arg, var));
        }
        if (integrated_args.empty())
          return make_num(0);
        ExprPtr r = integrated_args[0];
        for (size_t i = 1; i < integrated_args.size(); ++i)
          r = make_add(r, integrated_args[i]);
        return r;
      }

      // B. Linearity: Constant Extraction -> int(c * f(x)) dx = c * int(f(x))
      // dx
      if (app->head == "Mul") {
        // Due to canonical sorting, any constant Number will be at index 0
        if (dynamic_cast<const Number *>(app->args[0].get())) {
          std::vector<ExprPtr> remainder_args(app->args.begin() + 1,
                                              app->args.end());

          ExprPtr remainder;
          if (remainder_args.size() == 1)
            remainder = remainder_args[0];
          else {
            remainder = remainder_args[0];
            for (size_t i = 1; i < remainder_args.size(); ++i)
              remainder = make_mul(remainder, remainder_args[i]);
          }

          return make_mul(app->args[0], integrate(remainder, var));
        }
        // If no constant, we lack integration by parts in this basic heuristic.
        throw std::runtime_error("Integration Error: Non-linear/complex "
                                 "multiplication not yet supported.");
      }

      // C. The Power Rule -> int(x^n) dx
      if (app->head == "Pow") {
        if (auto *base_sym = dynamic_cast<const Symbol *>(app->args[0].get())) {
          if (base_sym->name == var) {
            if (auto *num_exp =
                    dynamic_cast<const Number *>(app->args[1].get())) {
              double n = num_exp->value;

              // Logarithmic Singularity: int(x^-1) dx = ln(x)
              if (n == -1.0) {
                return make_log(app->args[0]);
              }

              // Standard Power Rule: x^(n+1) / (n+1)
              ExprPtr new_exp = make_num(n + 1.0);
              return make_mul(
                  make_pow(make_num(n + 1.0), make_num(-1)), // 1/(n+1)
                  make_pow(app->args[0], new_exp)            // x^(n+1)
              );
            }
          }
        }
      }
    }

    throw std::runtime_error("Integration Engine Error: No heuristic matched "
                             "for the AST structure.");
  }
};