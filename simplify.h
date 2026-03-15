#pragma once
#include "expr.h"
#include <functional>
#include <unordered_map>
#include <vector>

namespace Simplifier {
using Rule = std::function<ExprPtr(const ExprPtr &)>;
ExprPtr simplify(const ExprPtr &e);

// Extract (coefficient, symbolic_base) from a term
inline std::pair<double, ExprPtr> extract_term(const ExprPtr &e) {
  if (!e)
    return {0.0, nullptr};
  if (auto *num = as_num(e))
    return {num->value, make_num(1)};
  if (auto *mul = as_apply(e, "Mul")) {
    double coeff = 1.0;
    std::vector<ExprPtr> symbolic;
    for (auto &arg : mul->args) {
      auto [c, t] = extract_term(arg);
      coeff *= c;
      if (!is_num(t, 1))
        symbolic.push_back(t);
    }
    if (symbolic.empty())
      return {coeff, make_num(1)};
    ExprPtr r = symbolic[0];
    for (size_t i = 1; i < symbolic.size(); ++i)
      r = make_mul(r, symbolic[i]);
    return {coeff, r};
  }
  return {1.0, e};
}

inline ExprPtr rule_combine_like_terms(const ExprPtr &e) {
  auto *add = as_apply(e, "Add");
  if (!add || add->args.size() < 2)
    return nullptr;

  std::unordered_map<const Expr *, double> coeff_map;
  std::vector<const Expr *> order;

  for (auto &arg : add->args) {
    auto [c, t] = extract_term(arg);
    auto it = coeff_map.find(t.get());
    if (it == coeff_map.end()) {
      coeff_map[t.get()] = c;
      order.push_back(t.get());
    } else
      it->second += c;
  }
  if (order.size() == add->args.size())
    return nullptr;

  ExprPtr result = make_num(0);
  for (auto *key : order) {
    double c = coeff_map[key];
    if (c == 0.0)
      continue;
    ExprPtr sym;
    for (auto &arg : add->args) {
      auto [c2, t2] = extract_term(arg);
      if (t2.get() == key) {
        sym = t2;
        break;
      }
    }
    result = make_add(result, make_mul(make_num(c), sym));
  }
  return result;
}

inline ExprPtr rule_combine_powers(const ExprPtr &e) {
  auto *mul = as_apply(e, "Mul");
  if (!mul || mul->args.size() < 2)
    return nullptr;

  std::unordered_map<const Expr *, ExprPtr> exponents_map;
  std::vector<const Expr *> order;
  std::unordered_map<const Expr *, ExprPtr> base_ptrs;

  for (auto &arg : mul->args) {
    ExprPtr base = arg;
    ExprPtr exp = make_num(1);
    if (auto *pow_node = as_apply(arg, "Pow")) {
      base = pow_node->args[0];
      exp = pow_node->args[1];
    }
    if (as_num(base)) {
      // Do not combine numbers via power if they are raw numbers, but make_pow
      // handles it anyway. It's safer to only combine symbols or complex
      // expressions, but let's just group identical pointers.
    }
    const Expr *key = base.get();
    auto it = exponents_map.find(key);
    if (it == exponents_map.end()) {
      exponents_map[key] = exp;
      order.push_back(key);
      base_ptrs[key] = base;
    } else {
      it->second = make_add(it->second, exp);
    }
  }

  if (order.size() == mul->args.size())
    return nullptr;

  ExprPtr result = make_num(1);
  for (auto *key : order) {
    ExprPtr exp = simplify(exponents_map[key]);
    ExprPtr base = base_ptrs[key];
    result = make_mul(result, make_pow(base, exp));
  }
  return result;
}

inline ExprPtr rule_nested_pow(const ExprPtr &e) {
  if (auto *outer_pow = as_apply(e, "Pow")) {
    if (auto *inner_pow = as_apply(outer_pow->args[0], "Pow")) {
      ExprPtr base = inner_pow->args[0];
      ExprPtr new_exp =
          simplify(make_mul(inner_pow->args[1], outer_pow->args[1]));
      return make_pow(base, new_exp);
    }
  }
  return nullptr;
}

inline ExprPtr rule_distribute(const ExprPtr &e) {
  if (auto *mul = as_apply(e, "Mul")) {
    for (size_t i = 0; i < mul->args.size(); ++i) {
      if (auto *add = as_apply(mul->args[i], "Add")) {
        std::vector<ExprPtr> distributed_args;
        for (const auto &add_term : add->args) {
          std::vector<ExprPtr> new_mul_args = mul->args;
          new_mul_args[i] = add_term;
          ExprPtr term = new_mul_args[0];
          for (size_t j = 1; j < new_mul_args.size(); ++j)
            term = make_mul(term, new_mul_args[j]);
          distributed_args.push_back(simplify(term));
        }
        if (distributed_args.empty())
          return make_num(0);
        ExprPtr res = distributed_args[0];
        for (size_t k = 1; k < distributed_args.size(); ++k)
          res = make_add(res, distributed_args[k]);
        return res;
      }
    }
  }
  return nullptr;
}

inline ExprPtr rule_exp_log(const ExprPtr &e) {
  auto *exp_node = as_apply(e, "Exp");
  if (!exp_node)
    return nullptr;
  auto *log_node = as_apply(exp_node->args[0], "Log");
  return log_node ? log_node->args[0] : nullptr;
}

static const std::vector<Rule> rules = {rule_combine_like_terms,
                                        rule_combine_powers, rule_nested_pow,
                                        rule_distribute, rule_exp_log};

inline ExprPtr apply_rules(ExprPtr e) {
  bool changed = true;
  int iter = 0;
  while (changed && iter < 100) {
    changed = false;
    for (auto &rule : rules) {
      if (ExprPtr t = rule(e)) {
        e = t;
        changed = true;
        break;
      }
    }
    iter++;
  }
  if (iter == 100)
    throw std::runtime_error("Simplifier infinite loop");
  return e;
}

inline ExprPtr simplify_children(const ExprPtr &e) {
  return map_children(e, [](const ExprPtr &arg) { return simplify(arg); });
}

inline ExprPtr simplify(const ExprPtr &e) {
  if (!e)
    return nullptr;
  return apply_rules(simplify_children(e));
}

inline ExprPtr expand_derivative(const ExprPtr &e) {
  if (!e)
    return nullptr;
  if (auto *d = as_apply(e, "Deriv")) {
    if (!dynamic_cast<const Symbol *>(d->args[0].get())) {
      ExprPtr exp = d->args[0]->derivative(d->deriv_var);
      for (int i = 1; i < d->deriv_order; i++)
        exp = expand_derivative(exp);
      return expand_derivative(exp);
    }
    return e;
  }
  return map_children(e, expand_derivative);
}
} // namespace Simplifier
