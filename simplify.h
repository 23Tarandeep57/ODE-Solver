#pragma once 
#include "expr.h"
#include <vector>
#include <functional>

namespace Simplifier {
    using Rule = std::function<ExprPtr(const ExprPtr&)>;

    inline std::pair<double, ExprPtr> extract_term(const ExprPtr& e) {
        if (!e) return {0.0, nullptr};

        if (auto* num = dynamic_cast<const Number*>(e.get())) 
            return {num->value, make_num(1)};
        
        if (auto* mul = dynamic_cast<const Mul*>(e.get())) {
            auto [c1, t1] = extract_term(mul->left);
            auto [c2, t2] = extract_term(mul->right);

            return {c1*c2, make_mul(t1, t2)};
        }

        if (auto* div = dynamic_cast<const Div*>(e.get())) {
            auto [c1, t1] = extract_term(div ->left);
            auto [c2, t2] = extract_term(div -> right);

            if (c2 == 0.0) throw std::runtime_error("Division by zero in coefficient");
            return {c1/c2, make_div(t1, t2)};
        }

        return {1.0, e};
    }


    inline ExprPtr rule_combine_like_terms(const ExprPtr& e) {
        auto* add = dynamic_cast<const Add*> (e.get());
        if (!add) return nullptr;

        auto [c1 ,t1] = extract_term(add->left);
        auto [c2, t2] = extract_term(add->right);

        if (t1.get() == t2.get()) return make_mul(make_num(c1+c2), t1);
        return nullptr;
    }

    inline ExprPtr rule_subtract_like_terms(const ExprPtr& e) {
        auto* sub = dynamic_cast<const Sub*> (e.get());
        if (!sub) return nullptr;

        auto [c1 ,t1] = extract_term(sub->left);
        auto [c2, t2] = extract_term(sub->right);

        if (t1.get() == t2.get()) return make_mul(make_num(c1-c2), t1);
        return nullptr;
    }
    
    inline ExprPtr rule_exp_log(const ExprPtr& e){
        auto* exp_node = dynamic_cast<const Exp*>(e.get());
        if (!exp_node) return nullptr;

        auto* log_node = dynamic_cast<const Log*>(exp_node->arg.get());
        if (!log_node) return nullptr;
        //exp(log(x)) = x
        return log_node->arg;
    }


    //rule registry
    static const std:: vector<Rule> rules = {
        rule_combine_like_terms,
        rule_subtract_like_terms,
        rule_exp_log
    };
    
    inline ExprPtr apply_rules(ExprPtr e) {
        bool changed = true;
        int iterations = 0;
        const int MAX_ITER = 100;

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
            throw std::runtime_error("Simplifer infinite loop detected");
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

        if (auto* sub = dynamic_cast<const Sub*>(e.get())) {
            auto l = simplify(sub->left);
            auto r = simplify(sub->right);
            if (l.get() != sub->left.get() || r.get() != sub->right.get())
                return make_sub(l, r);
            return e;
        }

        if (auto* div = dynamic_cast<const Div*>(e.get())) {
            auto l = simplify(div->left);
            auto r = simplify(div->right);
            if (l.get() != div->left.get() || r.get() != div->right.get())
                return make_div(l, r);
            return e;
        }

        if (auto* pow = dynamic_cast<const Pow*> (e.get())) {
            auto b = simplify(pow->base);
            auto ex = simplify(pow->exponent);
            if (b.get() != pow->base.get() || ex.get() != pow->exponent.get())
                return make_pow(b, ex);
        }
        
        if (auto* log = dynamic_cast<const Log*>(e.get())) {
            auto arg = simplify(log->arg);
            if (arg.get() != log->arg.get()) return make_log(arg);
            return e;
        }

        if (auto* exp = dynamic_cast<const Exp*>(e.get())) {
            auto arg = simplify(exp->arg);
            if (arg.get() != exp->arg.get()) return make_exp(arg);
            return e;
        }

        
        return e;
    }

    inline ExprPtr simplify(const ExprPtr& e) {
        if (!e) return nullptr;

        ExprPtr bottom_up_expr = simplify_children(e);
        return apply_rules(bottom_up_expr);
    }

}
