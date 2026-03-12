#pragma once

#include "expr.h"
#include <string>
#include <algorithm>
#include <stdexcept>


struct ODEMetrics{
    int order = -1; // -1 dependent variable is completely absent
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
            if (sym->name == dep_var) {
                return {0, 1, true}; // y is present, 0th order derivative
            }
            return {-1, 1, true};
        }

        if (auto* deriv = as_apply(e, "Deriv")) {
            if (auto* target_sym = dynamic_cast<const Symbol*>(deriv->args[0].get())) 
                if (target_sym->name == dep_var)
                    return {deriv->deriv_order, 1, true};
            return traverse(deriv->args[0], dep_var);
        }

        if (auto* add = as_apply(e, "Add")) {
            ODEMetrics res = {-1, 1, true};
            for (const auto& arg : add->args) {
                ODEMetrics A = traverse(arg, dep_var);
                res.is_linear = res.is_linear && A.is_linear;
                
                if (A.order > res.order) {
                    res.order = A.order;
                    res.degree = A.degree;
                } else if (A.order == res.order && A.order != -1) {
                    res.degree = std::max(res.degree, A.degree);
                }
            }
            return res;
        }

        if (auto* mul = as_apply(e, "Mul")) {
            ODEMetrics res = {-1, 1, true};
            int dep_count = 0; // how many factors contain the dep var
            
            for (const auto& arg : mul->args) {
                ODEMetrics A = traverse(arg, dep_var);
                res.is_linear = res.is_linear && A.is_linear;
                
                if (A.order != -1) {
                    dep_count++;
                    if (dep_count > 1) res.is_linear = false; // Cross-multiplication destroys linearity
                    
                    if (A.order > res.order) {
                        res.order = A.order;
                        res.degree = A.degree;
                    } else if (A.order == res.order) {
                        res.degree = res.degree + A.degree;
                    }
                }
            }
            return res;
        }

        if (auto* pow = as_apply(e, "Pow")) {
            ODEMetrics B = traverse(pow->args[0], dep_var);
            ODEMetrics E = traverse(pow->args[1], dep_var);
            
            // If the dependent variable is in the exponent eg 2^y strictly non-linear
            if (E.order != -1) return {std::max(B.order, E.order), 1, false};
            
            if (B.order != -1) {
                if (auto* num = dynamic_cast<const Number*>(pow->args[1].get())) {
                    // Check for negative exponent (from division)
                    if (num->value < 0) {
                        // dep var in denominator means non-linear
                        return {B.order, static_cast<int>(std::abs(num->value)) * B.degree, false};
                    }
                    bool is_int = (std::floor(num->value) == num->value);
                    bool linear = B.is_linear && is_int && (num->value == 1.0);
                    return {B.order, B.degree * static_cast<int>(num->value), linear};
                } else {
                    // Exponent is a function of x means transcendental degree
                    return {B.order, 9999, false}; 
                }
            }
            return {-1, 1, true};
        }

        if (auto* log = as_apply(e, "Log")) {
            ODEMetrics A = traverse(log->args[0], dep_var);
            return {A.order, 1, A.order == -1}; // lineaS if y is absent
        }
        if (auto* exp_node = as_apply(e, "Exp")) {
            ODEMetrics A = traverse(exp_node->args[0], dep_var);
            return {A.order, 1, A.order == -1}; // lineaS if y is absent
        }

        return {-1, 1, true};
    }

};
