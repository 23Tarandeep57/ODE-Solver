#pragma once
#include "expr.h"
#include "simplify.h"
#include <string>
#include <stdexcept>
#include <vector>

// y' + P(x)y = Q(x) boooooommmmmmmmm!!!!!!

struct StandardForm{
    ExprPtr P;
    ExprPtr Q;
};


class ODEExtractor {
public:
    static void flatten_terms(const ExprPtr& e, bool is_neg, std::vector<ExprPtr>& terms) {
        if (!e) return;
        if (auto* add = as_apply(e, "Add")) {
            for (const auto& arg : add->args) {
                flatten_terms(arg, is_neg, terms);
            }
        } else {
            if (is_neg)
                terms.push_back(Simplifier::simplify(make_mul(make_num(-1), e)));
            else 
                terms.push_back(e);
        }
    }


    static ExprPtr extract_factor(const ExprPtr& term, const ExprPtr& target) {
        if (expr_compare(term, target) == 0) return make_num(1); // term is exactly the target variable

        if (auto* mul = as_apply(term, "Mul")) {
            for (size_t i = 0; i < mul->args.size(); ++i) {
                if (ExprPtr res = extract_factor(mul->args[i], target)) {
                    ExprPtr product = res;
                    for (size_t j = 0; j < mul->args.size(); ++j) {
                        if (j != i) product = make_mul(product, mul->args[j]);
                    }
                    return product;
                }
            }
        }
        return nullptr; // target var not found
    }

public:
    static StandardForm extract_linear_first_order(const ExprPtr& eqn, const std::string& indep_var, const std::string& dep_var) {
        std::vector<ExprPtr> terms;
        flatten_terms(eqn, false, terms);

        ExprPtr target_y = make_sym(dep_var, indep_var);
        ExprPtr target_y_prime = make_derivative(target_y, indep_var, 1);

        ExprPtr A = make_num(0);
        ExprPtr B = make_num(0);
        ExprPtr C = make_num(0);

        for (const auto& term: terms) {
            if (ExprPtr coeff_A = extract_factor(term, target_y_prime)) {
                A = make_add(A, coeff_A);
            } else if (ExprPtr coeff_B = extract_factor(term, target_y)) {
                B = make_add(B, coeff_B);
            } else {
                C = make_add(C, term);
            }
        }

        A = Simplifier::simplify(A);
        B = Simplifier::simplify(B);
        C = Simplifier::simplify(C);

        //A(x) cannot be 0 if it's a 1st order ode
        if (auto* na = dynamic_cast<const Number*>(A.get())) {
            if (na->value == 0) {
                throw std::runtime_error("not a first-order ODE, coefficient of y' is zero");
            }
        }

        ExprPtr P = Simplifier::simplify(make_div(B, A));
        ExprPtr Q = Simplifier::simplify(make_div(C, A));
        return {P, Q};
    }
};