#pragma once

#include <memory>
#include <string>
#include <map>
#include <unordered_map>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <functional>


class Expr;
using ExprPtr = std::shared_ptr<const Expr>;

inline int  expr_compare(const ExprPtr& a, const ExprPtr& b);
inline bool expr_less(const ExprPtr& a, const ExprPtr& b);

class Expr {
public:
    virtual ~Expr() = default;
    virtual double evaluate(const std::map<std::string, double>& env) const = 0;
    virtual std::string to_string() const = 0;
    virtual ExprPtr derivative(const std::string& var) const = 0;

    virtual int type_rank() const = 0;
    virtual int compare_same_type(const Expr* other) const = 0;
};
    
class Number; class Symbol;
class Add; class Sub; class Mul; class Div; class Pow;
class Log; class Exp;

//They enforce:
//  - Algebraic simplification  (constant folding, identity/annihilation)
//   - Hash-consed expression pool  (DAG, not tree)

inline ExprPtr make_num(double v);
inline ExprPtr make_sym(const std::string& name);
inline ExprPtr make_add(ExprPtr a, ExprPtr b);
inline ExprPtr make_sub(ExprPtr a, ExprPtr b);
inline ExprPtr make_mul(ExprPtr a, ExprPtr b);
inline ExprPtr make_div(ExprPtr a, ExprPtr b);
inline ExprPtr make_pow(ExprPtr a, ExprPtr b);
inline ExprPtr make_log(ExprPtr a);
inline ExprPtr make_exp(ExprPtr a);

inline const Number* as_num(const ExprPtr& e);
inline bool is_num(const ExprPtr& e, double v);

class Number : public Expr {
public:
    double value;
    explicit Number(double v) : value(v) {}

    int type_rank() const override { return 1; }

    int compare_same_type(const Expr* other) const override {
        const auto *o = static_cast<const Number*> (other);
        if (value > o->value) return 1;
        else if (value < o->value) return -1;
        else return 0;
    }

    double evaluate(const std::map<std::string, double>&) const override {
        return value;
    }
    std::string to_string() const override {
        std::ostringstream os;
        os << value;
        return os.str();
    }
    ExprPtr derivative(const std::string&) const override {
        return make_num(0);
    }
};

class Symbol : public Expr {
public:
    std::string name;
    explicit Symbol(std::string n) : name(std::move(n)) {}

    int type_rank() const override { return 2; }

    int compare_same_type(const Expr* other) const override {
        const auto* o = static_cast<const Symbol*>(other);
        int cmp = name.compare(o->name);
        if (cmp < 0) return -1;
        if (cmp > 0) return 1;
        return 0;
    }

    double evaluate(const std::map<std::string, double>& env) const override {
        auto it = env.find(name);
        if (it == env.end())
            throw std::runtime_error("Undefined variable: " + name);
        return it->second;
    }
    std::string to_string() const override { return name; }
    ExprPtr derivative(const std::string& var) const override {
        return make_num(name == var ? 1 : 0);
    }
};

class Add : public Expr {
public:
    ExprPtr left, right;
    Add(ExprPtr l, ExprPtr r) : left(std::move(l)), right(std::move(r)) {}

    int type_rank() const override { return 3; }

    int compare_same_type(const Expr* other) const override {
        const auto* o = static_cast<const Add*>(other);
        int c = expr_compare(left, o->left);
        if (c != 0) return c;
        return expr_compare(right, o->right);
    }

    double evaluate(const std::map<std::string, double>& env) const override {
        return left->evaluate(env) + right->evaluate(env);
    }
    std::string to_string() const override {
        return "(" + left->to_string() + " + " + right->to_string() + ")";
    }
    ExprPtr derivative(const std::string& var) const override {
        return make_add(left->derivative(var), right->derivative(var));
    }
};

class Sub : public Expr {
public:
    ExprPtr left, right;
    Sub(ExprPtr l, ExprPtr r) : left(std::move(l)), right(std::move(r)) {}

    int type_rank() const override { return 4; }

    int compare_same_type(const Expr* other) const override {
        const auto* o = static_cast<const Sub*>(other);
        int c = expr_compare(left, o->left);
        if (c != 0) return c;
        return expr_compare(right, o->right);
    }

    double evaluate(const std::map<std::string, double>& env) const override {
        return left->evaluate(env) - right->evaluate(env);
    }
    std::string to_string() const override {
        return "(" + left->to_string() + " - " + right->to_string() + ")";
    }
    ExprPtr derivative(const std::string& var) const override {
        return make_sub(left->derivative(var), right->derivative(var));
    }
};

class Mul : public Expr {
public:
    ExprPtr left, right;
    Mul(ExprPtr l, ExprPtr r) : left(std::move(l)), right(std::move(r)) {}

    int type_rank() const override { return 5; }

    int compare_same_type(const Expr* other) const override {
        const auto* o = static_cast<const Mul*>(other);
        int c = expr_compare(left, o->left);
        if (c != 0) return c;
        return expr_compare(right, o->right);
    }

    double evaluate(const std::map<std::string, double>& env) const override {
        return left->evaluate(env) * right->evaluate(env);
    }
    std::string to_string() const override {
        return "(" + left->to_string() + " * " + right->to_string() + ")";
    }
    // Product rule: d(u*v) = u*dv + du*v
    ExprPtr derivative(const std::string& var) const override {
        return make_add(
            make_mul(left, right->derivative(var)),
            make_mul(left->derivative(var), right)
        );
    }
};

class Div : public Expr {
public:
    ExprPtr left, right;
    Div(ExprPtr l, ExprPtr r) : left(std::move(l)), right(std::move(r)) {}

    int type_rank() const override { return 6; }

    int compare_same_type(const Expr* other) const override {
        const auto* o = static_cast<const Div*>(other);
        int c = expr_compare(left, o->left);
        if (c != 0) return c;
        return expr_compare(right, o->right);
    }

    double evaluate(const std::map<std::string, double>& env) const override {
        double d = right->evaluate(env);
        if (d == 0.0) throw std::runtime_error("Division by zero");
        return left->evaluate(env) / d;
    }
    std::string to_string() const override {
        return "(" + left->to_string() + " / " + right->to_string() + ")";
    }
    // Quotient rule: d(u/v) = (du*v - u*dv) / v^2
    ExprPtr derivative(const std::string& var) const override {
        return make_div(
            make_sub(make_mul(left->derivative(var), right),
                     make_mul(left, right->derivative(var))),
            make_pow(right, make_num(2))
        );
    }
};


class Pow : public Expr {
public:
    ExprPtr base, exponent;
    Pow(ExprPtr b, ExprPtr e) : base(std::move(b)), exponent(std::move(e)) {}

    int type_rank() const override { return 7; }

    int compare_same_type(const Expr* other) const override {
        const auto* o = static_cast<const Pow*>(other);
        int c = expr_compare(base, o->base);
        if (c != 0) return c;
        return expr_compare(exponent, o->exponent);
    }

    double evaluate(const std::map<std::string, double>& env) const override {
        return std::pow(base->evaluate(env), exponent->evaluate(env));
    }
    std::string to_string() const override {
        return "(" + base->to_string() + " ^ " + exponent->to_string() + ")";
    }
    // Constant exponent:  d(u^n) = n * u^(n-1) * u'
    // Variable exponent (logarithmic differentiation):
    //   d(u^v) = u^v * ( v' * ln(u)  +  v * u' / u )
    ExprPtr derivative(const std::string& var) const override {
        auto* n = as_num(exponent);
        if (n) {
            // const exponent power rule
            return make_mul(
                make_mul(exponent, make_pow(base, make_num(n->value - 1))),
                base->derivative(var)
            );
        }
        // general case via logarithmic differentiation
        ExprPtr uv = make_pow(base, exponent);
        return make_mul(uv,
            make_add(
                make_mul(exponent->derivative(var), make_log(base)),
                make_mul(exponent, make_div(base->derivative(var), base))
            )
        );
    }
};


class Log : public Expr {
public:
    ExprPtr arg;
    explicit Log(ExprPtr a) : arg(std::move(a)) {}

    int type_rank() const override { return 8; }

    int compare_same_type(const Expr* other) const override {
        const auto* o = static_cast<const Log*>(other);
        return expr_compare(arg, o->arg);
    }

    double evaluate(const std::map<std::string, double>& env) const override {
        double v = arg->evaluate(env);
        if (v <= 0.0) throw std::runtime_error("Log of non-positive value");
        return std::log(v);
    }
    std::string to_string() const override {
        return "ln(" + arg->to_string() + ")";
    }
    // d(ln u) = u' / u
    ExprPtr derivative(const std::string& var) const override {
        return make_div(arg->derivative(var), arg);
    }
};

class Exp : public Expr {
public:
    ExprPtr arg;
    explicit Exp(ExprPtr a) : arg(std::move(a)) {}

    int type_rank() const override { return 9; }

    int compare_same_type(const Expr* other) const override {
        const auto* o = static_cast<const Exp*>(other);
        return expr_compare(arg, o->arg);
    }

    double evaluate(const std::map<std::string, double>& env) const override {
        return std::exp(arg->evaluate(env));
    }
    std::string to_string() const override {
        return "exp(" + arg->to_string() + ")";
    }
    // d(e^u) = e^u * u'
    ExprPtr derivative(const std::string& var) const override {
        return make_mul(make_exp(arg), arg->derivative(var));
    }
};

namespace detail {

struct BinaryKeyHash {
    size_t operator()(const std::tuple<int, const Expr*, const Expr*>& k) const {
        size_t h = std::hash<int>{}(std::get<0>(k));
        h ^= std::hash<const void*>{}(std::get<1>(k)) * 2654435761u;
        h ^= std::hash<const void*>{}(std::get<2>(k)) * 40503u;
        return h;
    }
};
struct UnaryKeyHash {
    size_t operator()(const std::pair<int, const Expr*>& k) const {
        size_t h = std::hash<int>{}(k.first);
        h ^= std::hash<const void*>{}(k.second) * 2654435761u;
        return h;
    }
};

using BinaryKey = std::tuple<int, const Expr*, const Expr*>;
using UnaryKey  = std::pair<int, const Expr*>;

inline auto& binary_pool() {
    static std::unordered_map<BinaryKey, std::weak_ptr<const Expr>, BinaryKeyHash> p;
    return p;
}
inline auto& unary_pool() {
    static std::unordered_map<UnaryKey, std::weak_ptr<const Expr>, UnaryKeyHash> p;
    return p;
}

enum NodeTag { TAG_ADD, TAG_SUB, TAG_MUL, TAG_DIV, TAG_POW, TAG_LOG, TAG_EXP };

template <typename NodeT>
inline ExprPtr pool_binary(int tag, ExprPtr a, ExprPtr b) {
    BinaryKey key{tag, a.get(), b.get()};
    auto& pool = binary_pool();
    auto it = pool.find(key);
    if (it != pool.end())
        if (auto sp = it->second.lock()) return sp;
    auto ptr = std::make_shared<const NodeT>(std::move(a), std::move(b));
    pool[key] = ptr;
    return ptr;
}

template <typename NodeT>
inline ExprPtr pool_unary(int tag, ExprPtr a) {
    UnaryKey key{tag, a.get()};
    auto& pool = unary_pool();
    auto it = pool.find(key);
    if (it != pool.end())
        if (auto sp = it->second.lock()) return sp;
    auto ptr = std::make_shared<const NodeT>(std::move(a));
    pool[key] = ptr;
    return ptr;
}

} // namespace detail


inline int expr_compare(const ExprPtr& a, const ExprPtr& b) {
    if (a.get() == b.get()) return 0;
    int ra = a->type_rank();
    int rb = b->type_rank();
    if (ra < rb) return -1;
    if (ra > rb) return  1;
    return a->compare_same_type(b.get());
}

inline bool expr_less(const ExprPtr& a, const ExprPtr& b) {
    return expr_compare(a, b) < 0;
}

inline const Number* as_num(const ExprPtr& e) {
    return dynamic_cast<const Number*>(e.get());
}
inline bool is_num(const ExprPtr& e, double v) {
    auto* n = as_num(e);
    return n && n->value == v;
}

inline ExprPtr make_num(double v) {
    static std::unordered_map<double, std::weak_ptr<const Expr>> pool;
    auto it = pool.find(v);
    if (it != pool.end())
        if (auto sp = it->second.lock()) return sp;
    auto ptr = std::make_shared<const Number>(v);
    pool[v] = ptr;
    return ptr;
}

inline ExprPtr make_sym(const std::string& name) {
    static std::unordered_map<std::string, std::weak_ptr<const Expr>> pool;
    auto it = pool.find(name);
    if (it != pool.end())
        if (auto sp = it->second.lock()) return sp;
    auto ptr = std::make_shared<const Symbol>(name);
    pool[name] = ptr;
    return ptr;
}

inline ExprPtr make_add(ExprPtr a, ExprPtr b) {
    // Enforce Canonical Order (constants left, symbols lexicographic)
    if (expr_less(b, a)) std::swap(a, b);

    // Simplification
    auto *na = as_num(a), *nb = as_num(b);
    if (na && nb) return make_num(na->value + nb->value);   // constant fold
    if (is_num(a, 0)) return b;                              // 0 + b → b

    return detail::pool_binary<Add>(detail::TAG_ADD, std::move(a), std::move(b));
}

inline ExprPtr make_sub(ExprPtr a, ExprPtr b) {
    auto *na = as_num(a), *nb = as_num(b);
    if (na && nb) return make_num(na->value - nb->value);  // constant fold
    if (is_num(b, 0)) return a;                             // a - 0 → a
    if (a.get() == b.get()) return make_num(0);             // a - a → 0  (DAG ptr eq)
    return detail::pool_binary<Sub>(detail::TAG_SUB, std::move(a), std::move(b));
}

inline ExprPtr make_mul(ExprPtr a, ExprPtr b) {
    // 1. Enforce Canonical Order (constants left, symbols lexicographic)
    if (expr_less(b, a)) std::swap(a, b);

    // 2. Algebraic Simplification
    auto *na = as_num(a), *nb = as_num(b);
    if (na && nb) return make_num(na->value * nb->value);   // constant fold
    if (is_num(a, 0)) return make_num(0);                    // 0 * _ → 0
    if (is_num(a, 1)) return b;                              // 1 * b → b

    // 3. Hash Pooling
    return detail::pool_binary<Mul>(detail::TAG_MUL, std::move(a), std::move(b));
}

inline ExprPtr make_div(ExprPtr a, ExprPtr b) {
    auto *na = as_num(a), *nb = as_num(b);
    if (na && nb && nb->value != 0) return make_num(na->value / nb->value);
    if (is_num(a, 0)) return make_num(0);                   // 0 / b → 0
    if (is_num(b, 1)) return a;                             // a / 1 → a
    if (a.get() == b.get()) return make_num(1);             // a / a → 1  (DAG ptr eq)
    return detail::pool_binary<Div>(detail::TAG_DIV, std::move(a), std::move(b));
}

inline ExprPtr make_pow(ExprPtr a, ExprPtr b) {
    auto *na = as_num(a), *nb = as_num(b);
    if (na && nb) return make_num(std::pow(na->value, nb->value));  // constant fold
    if (is_num(b, 0)) return make_num(1);                   // a^0 → 1
    if (is_num(b, 1)) return a;                             // a^1 → a
    if (is_num(a, 1)) return make_num(1);                   // 1^b → 1
    return detail::pool_binary<Pow>(detail::TAG_POW, std::move(a), std::move(b));
}


inline ExprPtr make_log(ExprPtr a) {
    auto* na = as_num(a);
    if (na && na->value > 0) return make_num(std::log(na->value));  // constant fold
    if (auto* ea = dynamic_cast<const Exp*>(a.get()))                // ln(e^x) → x
        return ea->arg;
    return detail::pool_unary<Log>(detail::TAG_LOG, std::move(a));
}

inline ExprPtr make_exp(ExprPtr a) {
    auto* na = as_num(a);
    if (na) return make_num(std::exp(na->value));                    // constant fold
    if (auto* la = dynamic_cast<const Log*>(a.get()))                // e^(ln x) → x
        return la->arg;
    return detail::pool_unary<Exp>(detail::TAG_EXP, std::move(a));
}
