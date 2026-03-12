#pragma once 
#include "expr.h"
#include <vector>
#include <functional>
#include <unordered_map>

namespace Simplifier {
    using Rule = std::function<ExprPtr(const ExprPtr&)>;

    // Extract (coefficient, symbolic_base) from a term
    inline std::pair<double, ExprPtr> extract_term(const ExprPtr& e) {
        if (!e) return {0.0, nullptr};
        if (auto* num = as_num(e)) return {num->value, make_num(1)};
        if (auto* mul = as_apply(e, "Mul")) {
            double coeff = 1.0;
            std::vector<ExprPtr> symbolic;
            for (auto& arg : mul->args) {
                auto [c, t] = extract_term(arg);
                coeff *= c;
                if (!is_num(t, 1)) symbolic.push_back(t);
            }
            if (symbolic.empty()) return {coeff, make_num(1)};
            ExprPtr r = symbolic[0];
            for (size_t i = 1; i < symbolic.size(); ++i) r = make_mul(r, symbolic[i]);
            return {coeff, r};
        }
        return {1.0, e};
    }

    inline ExprPtr rule_combine_like_terms(const ExprPtr& e) {
        auto* add = as_apply(e, "Add");
        if (!add || add->args.size() < 2) return nullptr;

        std::unordered_map<const Expr*, double> coeff_map;
        std::vector<const Expr*> order;

        for (auto& arg : add->args) {
            auto [c, t] = extract_term(arg);
            auto it = coeff_map.find(t.get());
            if (it == coeff_map.end()) { coeff_map[t.get()] = c; order.push_back(t.get()); }
            else it->second += c;
        }
        if (order.size() == add->args.size()) return nullptr;

        ExprPtr result = make_num(0);
        for (auto* key : order) {
            double c = coeff_map[key];
            if (c == 0.0) continue;
            ExprPtr sym;
            for (auto& arg : add->args) { auto [c2,t2] = extract_term(arg); if (t2.get()==key) { sym=t2; break; } }
            result = make_add(result, make_mul(make_num(c), sym));
        }
        return result;
    }

    inline ExprPtr rule_exp_log(const ExprPtr& e) {
        auto* exp_node = as_apply(e, "Exp");
        if (!exp_node) return nullptr;
        auto* log_node = as_apply(exp_node->args[0], "Log");
        return log_node ? log_node->args[0] : nullptr;
    }

    static const std::vector<Rule> rules = { rule_combine_like_terms, rule_exp_log };
    
    inline ExprPtr apply_rules(ExprPtr e) {
        bool changed = true;
        int iter = 0;
        while (changed && iter < 100) {
            changed = false;
            for (auto& rule : rules) {
                if (ExprPtr t = rule(e)) { e = t; changed = true; break; }
            }
            iter++;
        }
        if (iter == 100) throw std::runtime_error("Simplifier infinite loop");
        return e;
    }

    ExprPtr simplify(const ExprPtr& e);

    inline ExprPtr simplify_children(const ExprPtr& e) {
        return map_children(e, [](const ExprPtr& arg) { return simplify(arg); });
    }

    inline ExprPtr simplify(const ExprPtr& e) {
        if (!e) return nullptr;
        return apply_rules(simplify_children(e));
    }

    inline ExprPtr expand_derivative(const ExprPtr& e) {
        if (!e) return nullptr;
        if (auto* d = as_apply(e, "Deriv")) {
            if (!dynamic_cast<const Symbol*>(d->args[0].get())) {
                ExprPtr exp = d->args[0]->derivative(d->deriv_var);
                for (int i = 1; i < d->deriv_order; i++) exp = expand_derivative(exp);
                return expand_derivative(exp);
            }
            return e;
        }
        return map_children(e, expand_derivative);
    }
} // namespace Simplifier
