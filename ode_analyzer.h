#pragma once

#include "expr.h"
#include <string>
#include <algorithm>
#include <stdexcept>


struct ODEMetrics{
    int order = -1; // -1 dependednt variable is completely absent
    int degree = 1;
    bool is_linear = true;
};

class ODEAnalyzer {

public:
    static ODEMetrics analyze (const ExprPtr& e, const std::string& dep_var) {
        if (!e) return ODEMetrics();
        return traverse(e, dep_var);
    }


private:
    static ODEMetrics traverse(const ExprPtr& e, const std::string& dep_var) {
        if (dynamic_cast<const Number*>(e.get())) 
            return {-1, 1, true};
        
        if (auto* sym = dynamic_cast<const Symbol*>(e.get())) {
            if (sym -> name == dep_var) {
                return {0, 1, true}; // y is present 0th order derivative
            }
            return {-1, 1, true};
        }

        if (auto * deriv = dynamic_cast<const Derivative*>(e.get())) {
            if (auto* target_sym = dynamic_cast<const Symbol*> (deriv-> target.get())) 
                if (target_sym->name == dep_var)
                    return {deriv->order, 1, true};
            return traverse(deriv->target, dep_var);
        }

        if (dynamic_cast<const Add*>(e.get()) || dynamic_cast<const Sub*>(e.get())) {
            ExprPtr left = dynamic_cast<const Add*>(e.get()) ? static_cast<const Add*>(e.get())->left : static_cast<const Sub*>(e.get())->left;
            ExprPtr right = dynamic_cast<const Add*>(e.get()) ? static_cast<const Add*>(e.get())->right : static_cast<const Sub*>(e.get())->right;
            
            ODEMetrics L = traverse(left, dep_var);
            ODEMetrics R = traverse(right, dep_var);
            
            ODEMetrics res;
            res.is_linear = L.is_linear && R.is_linear;
            
            if (L.order > R.order) {
                res.order = L.order;
                res.degree = L.degree;
            } else if (R.order > L.order) {
                res.order = R.order;
                res.degree = R.degree;
            } else {
                res.order = L.order;
                res.degree = std::max(L.degree, R.degree);
            }
            return res;
        }

        if (auto* mul = dynamic_cast<const Mul*>(e.get())) {
            ODEMetrics L = traverse(mul->left, dep_var);
            ODEMetrics R = traverse(mul->right, dep_var);
            
            ODEMetrics res;
            // Cross-multiplication of the dependent variable (e.g., y * y') destroys linearity
            res.is_linear = L.is_linear && R.is_linear && !(L.order != -1 && R.order != -1);
            
            if (L.order > R.order) {
                res.order = L.order;
                res.degree = L.degree;
            } else if (R.order > L.order) {
                res.order = R.order;
                res.degree = R.degree;
            } else {
                res.order = L.order;
                // If orders are equal and both contain y, exponents add: (y')^2 * (y')^3 -> (y')^5
                res.degree = (L.order != -1) ? (L.degree + R.degree) : 1; 
            }
            return res;
        }

        if (auto* div = dynamic_cast<const Div*>(e.get())) {
            ODEMetrics L = traverse(div->left, dep_var);
            ODEMetrics R = traverse(div->right, dep_var);
            
            // If the dependent variable is in the denominator, it is non-linear
            bool linear = L.is_linear && R.is_linear && (R.order == -1);
            
            ODEMetrics res = {L.order, L.degree, linear};
            if (R.order > L.order) {
                res.order = R.order;
                res.degree = -R.degree; // Fractional degree (algebraic singularity)
            }
            return res;
        }

        if (auto* pow = dynamic_cast<const Pow*>(e.get())) {
            ODEMetrics B = traverse(pow->base, dep_var);
            ODEMetrics E = traverse(pow->exponent, dep_var);
            
            // If the dependent variable is in the exponent (e.g., 2^y), strictly non-linear
            if (E.order != -1) return {std::max(B.order, E.order), 1, false};
            
            if (B.order != -1) {
                if (auto* num = dynamic_cast<const Number*>(pow->exponent.get())) {
                    // Check if exponent is an integer. If not it's a fractional degree (e.g., sqrt(y'))
                    bool is_int = (std::floor(num->value) == num->value);
                    bool linear = B.is_linear && is_int && (num->value == 1.0);
                    return {B.order, B.degree * static_cast<int>(num->value), linear};
                } else {
                    // Exponent is a function of x (e.g., (y')^x) -> transcendental degree
                    return {B.order, 9999, false}; 
                }
            }
            return {-1, 1, true};
        }

        if (auto* log = dynamic_cast<const Log*>(e.get())) {
            ODEMetrics A = traverse(log->arg, dep_var);
            return {A.order, 1, A.order == -1}; // Linear ONLY if y is absent
        }
        if (auto* exp = dynamic_cast<const Exp*>(e.get())) {
            ODEMetrics A = traverse(exp->arg, dep_var);
            return {A.order, 1, A.order == -1}; // Linear ONLY if y is absent
        }

        return {-1, 1, true};
    }

};

