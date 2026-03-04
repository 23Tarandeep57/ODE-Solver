#pragma once 
#include "expr.h"
#include <vector>
#include <function>

namespace Simplified {
    using Rule = std::function<ExprPtr(const ExprPtr&)>;

    inline ExprPtr rule_add_same(const ExprPtr& e) {
        auto add* dynamic_cast<const Add*> (e.get());
        if (!add) return nullptr;

        // a+a = 2*a

        if (add->left.get() == add->right.get()) {
            return make_mul(make_num(2), add->left);
        }
        return nullptr;
    }

    inline ExprPtr rule_exp_log(const ExprPtr &e) {
        auto* exp_node = dynamic_cast<const Exp*>(e.get());
        if (!exp_node) return nullptr;

        auto* log_node = dynamic_casr<const Log*>(exp_node->arg.get());
        if (!log_node) return nullptr;
        //exp(log(x)) = x
        return log_node->arg;
    }


    //rule registry
    static const std:: vector<Rule> rules = {
        rule_add_same,
        rule_exp_log
    };
    
    inline ExprPtr apply_rules(ExprPtr e) {
        bool changed = true;
        int iterations = 0;
        const MAX_ITER = 100;

        while (changed && iterations < MAX_ITER) {
            changed = false;
            for (const auto& rule: rules) {
                if (ExprPtr transformed = rule(e)){
                    e= transformed;
                    changed = true;
                    break; // restart rule evaluation one new expression
                }
            }
            iterations++;
        }
        if (iterations == MAX_ITER) {
            throw std::runtime_error("Simplifer infinite loop detected")
        }

        return e;

    }



    ExprPtr simplify(const ExprPtr& e) ;

    inline ExprPtr simplify_children(const ExprPtr& e) {
        if (auto* add = dynamic_cast<const Add*>(e.get())) {
            auto l = simplify(add->left);
            auto r = simplify(add -> right);
            if (l.get() != add->left.get() || r.get() != add->right.get()) 
                return make_add(l,r);
            return e;

        }

        if (auto* mul = dynamic_cast<const Mul*>(e.get())) {
            auto l = simplify(mul->left);
            auto r = simplify(mul->right);
            if (l.get() != mul->left.get() || r.get() != mul->right.get())
                return make_mul(l, r);
            return e;
        }

        // pending for other functions
        return e
    }

    inline ExprPtr simplify(const ExprPtr& e) {
        if (!e) return nullptr;

        ExprPtr bottom_up_expr = simplify_children(e);
        return apply_rules(bottom_up_expr);
    }

}
