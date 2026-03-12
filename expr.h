#pragma once

#include <memory>
#include <string>
#include <map>
#include <unordered_map>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <algorithm>
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

class Number; class Symbol; class Apply;

inline ExprPtr make_num(double v);
inline ExprPtr make_sym(const std::string& name, const std::string& dep = "");
inline ExprPtr make_apply(const std::string& head, std::vector<ExprPtr> args,
                          const std::string& dv = "", int dord = 0);
inline ExprPtr make_add(ExprPtr a, ExprPtr b);
inline ExprPtr make_sub(ExprPtr a, ExprPtr b);
inline ExprPtr make_mul(ExprPtr a, ExprPtr b);
inline ExprPtr make_div(ExprPtr a, ExprPtr b);
inline ExprPtr make_pow(ExprPtr a, ExprPtr b);
inline ExprPtr make_log(ExprPtr a);
inline ExprPtr make_exp(ExprPtr a);
inline ExprPtr make_derivative(ExprPtr expr, const std::string& var, int order = 1);

inline const Number* as_num(const ExprPtr& e);
inline bool is_num(const ExprPtr& e, double v);
inline const Apply* as_apply(const ExprPtr& e);
inline const Apply* as_apply(const ExprPtr& e, const std::string& head);

class Number : public Expr {
public:
    double value;
    explicit Number(double v) : value(v) {}
    int type_rank() const override { return 1; }
    int compare_same_type(const Expr* other) const override {
        auto *o = static_cast<const Number*>(other);
        return (value > o->value) - (value < o->value);
    }
    double evaluate(const std::map<std::string, double>&) const override { return value; }
    std::string to_string() const override {
        std::ostringstream os; os << value; return os.str();
    }
    ExprPtr derivative(const std::string&) const override { return make_num(0); }
};

class Symbol : public Expr {
public:
    std::string name, depends_on;
    explicit Symbol(std::string n, std::string dep = "")
        : name(std::move(n)), depends_on(std::move(dep)) {}
    int type_rank() const override { return 2; }
    int compare_same_type(const Expr* other) const override {
        auto* o = static_cast<const Symbol*>(other);
        int c = name.compare(o->name);
        return c ? c : depends_on.compare(o->depends_on);
    }
    double evaluate(const std::map<std::string, double>& env) const override {
        auto it = env.find(name);
        if (it == env.end()) throw std::runtime_error("Undefined variable: " + name);
        return it->second;
    }
    std::string to_string() const override { return name; }
    ExprPtr derivative(const std::string& var) const override {
        if (name == var) return make_num(1);
        if (depends_on == var) return make_derivative(make_sym(name, depends_on), var);
        return make_num(0);
    }
};

// Heads: "Add", "Mul", "Pow", "Log", "Exp", "Deriv"
// Sub/Div desugared: Sub(a,b) → Add(a, Mul(-1,b)), Div(a,b) → Mul(a, Pow(b,-1))

class Apply : public Expr {
public:
    std::string head;
    std::vector<ExprPtr> args;
    std::string deriv_var;   
    int deriv_order = 0;     

    Apply(std::string h, std::vector<ExprPtr> a, std::string dv = "", int do_ = 0)
        : head(std::move(h)), args(std::move(a)), deriv_var(std::move(dv)), deriv_order(do_) {}

    int type_rank() const override { return 3; }

    int compare_same_type(const Expr* other) const override {
        auto* o = static_cast<const Apply*>(other);
        int c = head.compare(o->head);
        if (c) return c;
        if (head == "Deriv") {
            c = deriv_var.compare(o->deriv_var);
            if (c) return c;
            if (deriv_order != o->deriv_order) return deriv_order < o->deriv_order ? -1 : 1;
        }
        size_t n = std::min(args.size(), o->args.size());
        for (size_t i = 0; i < n; ++i) {
            c = expr_compare(args[i], o->args[i]);
            if (c) return c;
        }
        if (args.size() != o->args.size()) return args.size() < o->args.size() ? -1 : 1;
        return 0;
    }

    double evaluate(const std::map<std::string, double>& env) const override {
        if (head == "Add") { double s=0; for (auto& a:args) s+=a->evaluate(env); return s; }
        if (head == "Mul") { double p=1; for (auto& a:args) p*=a->evaluate(env); return p; }
        if (head == "Pow") return std::pow(args[0]->evaluate(env), args[1]->evaluate(env));
        if (head == "Log") { double v=args[0]->evaluate(env); if(v<=0) throw std::runtime_error("Log of non-positive"); return std::log(v); }
        if (head == "Exp") return std::exp(args[0]->evaluate(env));
        if (head == "Deriv") throw std::runtime_error("Cannot evaluate symbolic derivative d/d" + deriv_var);
        throw std::runtime_error("Unknown head: " + head);
    }

    std::string to_string() const override {
        auto join = [&](const std::string& sep) {
            std::string s = "(";
            for (size_t i = 0; i < args.size(); ++i) { if (i) s += sep; s += args[i]->to_string(); }
            return s + ")";
        };
        if (head == "Add") return join(" + ");
        if (head == "Mul") return join(" * ");
        if (head == "Pow") return "(" + args[0]->to_string() + " ^ " + args[1]->to_string() + ")";
        if (head == "Log") return "ln(" + args[0]->to_string() + ")";
        if (head == "Exp") return "exp(" + args[0]->to_string() + ")";
        if (head == "Deriv") {
            if (deriv_order == 1) return "d(" + args[0]->to_string() + ")/d" + deriv_var;
            return "d^" + std::to_string(deriv_order) + "(" + args[0]->to_string() + ")/d" + deriv_var + "^" + std::to_string(deriv_order);
        }
        return head + "(...)";
    }

    ExprPtr derivative(const std::string& var) const override;
};

namespace detail {
using ApplyKey = std::tuple<std::string, std::vector<const Expr*>, std::string, int>;
struct ApplyKeyHash {
    size_t operator()(const ApplyKey& k) const {
        size_t h = std::hash<std::string>{}(std::get<0>(k));
        for (auto* p : std::get<1>(k)) h ^= std::hash<const void*>{}(p) + 0x9e3779b9 + (h<<6) + (h>>2);
        h ^= std::hash<std::string>{}(std::get<2>(k)) * 2654435761u;
        h ^= std::hash<int>{}(std::get<3>(k)) * 40503u;
        return h;
    }
};
inline auto& apply_pool() {
    static std::unordered_map<ApplyKey, std::weak_ptr<const Expr>, ApplyKeyHash> p;
    return p;
}
} // namespace detail

template<typename Map, typename Key>
ExprPtr try_pool(Map& pool, const Key& key) {
    auto it = pool.find(key);
    if (it != pool.end()) if (auto sp = it->second.lock()) return sp;
    return nullptr;
}

inline int expr_compare(const ExprPtr& a, const ExprPtr& b) {
    if (a.get() == b.get()) return 0;
    int ra = a->type_rank(), rb = b->type_rank();
    if (ra != rb) return ra < rb ? -1 : 1;
    return a->compare_same_type(b.get());
}
inline bool expr_less(const ExprPtr& a, const ExprPtr& b) { return expr_compare(a, b) < 0; }

inline const Number* as_num(const ExprPtr& e) { return dynamic_cast<const Number*>(e.get()); }
inline bool is_num(const ExprPtr& e, double v) { auto* n = as_num(e); return n && n->value == v; }
inline const Apply* as_apply(const ExprPtr& e) { return dynamic_cast<const Apply*>(e.get()); }
inline const Apply* as_apply(const ExprPtr& e, const std::string& head) {
    auto* a = as_apply(e); return (a && a->head == head) ? a : nullptr;
}

inline ExprPtr rebuild_apply(const Apply* app, std::vector<ExprPtr> new_args) {
    if (app->head == "Add") { ExprPtr r=new_args[0]; for (size_t i=1;i<new_args.size();++i) r=make_add(r,new_args[i]); return r; }
    if (app->head == "Mul") { ExprPtr r=new_args[0]; for (size_t i=1;i<new_args.size();++i) r=make_mul(r,new_args[i]); return r; }
    if (app->head == "Pow")   return make_pow(new_args[0], new_args[1]);
    if (app->head == "Log")   return make_log(new_args[0]);
    if (app->head == "Exp")   return make_exp(new_args[0]);
    if (app->head == "Deriv") return make_derivative(new_args[0], app->deriv_var, app->deriv_order);
    return make_apply(app->head, std::move(new_args), app->deriv_var, app->deriv_order);
}

inline ExprPtr map_children(const ExprPtr& e, const std::function<ExprPtr(const ExprPtr&)>& fn) {
    auto* app = as_apply(e);
    if (!app) return e;
    bool changed = false;
    std::vector<ExprPtr> new_args;
    new_args.reserve(app->args.size());
    for (const auto& arg : app->args) {
        ExprPtr t = fn(arg);
        if (t.get() != arg.get()) changed = true;
        new_args.push_back(t);
    }
    return changed ? rebuild_apply(app, std::move(new_args)) : e;
}

inline ExprPtr make_apply(const std::string& head, std::vector<ExprPtr> args,
                          const std::string& dv, int dord) {
    std::vector<const Expr*> kp; kp.reserve(args.size());
    for (auto& a : args) kp.push_back(a.get());
    detail::ApplyKey key{head, kp, dv, dord};
    auto& pool = detail::apply_pool();
    if (auto sp = try_pool(pool, key)) return sp;
    auto ptr = std::make_shared<const Apply>(head, std::move(args), dv, dord);
    pool[key] = ptr;
    return ptr;
}

inline ExprPtr make_num(double v) {
    static std::unordered_map<double, std::weak_ptr<const Expr>> pool;
    if (auto sp = try_pool(pool, v)) return sp;
    auto ptr = std::make_shared<const Number>(v);
    pool[v] = ptr;
    return ptr;
}

inline ExprPtr make_sym(const std::string& name, const std::string& dep) {
    static std::unordered_map<std::string, std::weak_ptr<const Expr>> pool;
    std::string key = name + "@" + dep;
    if (auto sp = try_pool(pool, key)) return sp;
    auto ptr = std::make_shared<const Symbol>(name, dep);
    pool[key] = ptr;
    return ptr;
}

inline ExprPtr make_nary(const std::string& head, ExprPtr a, ExprPtr b,
                         double identity, double absorb_check = std::nan("")) {
    std::vector<ExprPtr> parts;
    auto collect = [&](const ExprPtr& e) {
        if (auto* app = as_apply(e, head))
            for (auto& arg : app->args) parts.push_back(arg);
        else parts.push_back(e);
    };
    collect(a); collect(b);

    double accum = identity;
    std::vector<ExprPtr> symbolic;
    for (auto& p : parts) {
        if (auto* n = as_num(p))
            accum = (head == "Add") ? accum + n->value : accum * n->value;
        else symbolic.push_back(p);
    }
    if (!std::isnan(absorb_check) && accum == absorb_check) return make_num(absorb_check);

    if (symbolic.empty()) return make_num(accum);

    if (accum != identity) symbolic.push_back(make_num(accum));

    if (symbolic.size() == 1) return symbolic[0];

    std::sort(symbolic.begin(), symbolic.end(), expr_less);
    return make_apply(head, std::move(symbolic));
}

inline ExprPtr make_add(ExprPtr a, ExprPtr b) { return make_nary("Add", std::move(a), std::move(b), 0.0); }
inline ExprPtr make_mul(ExprPtr a, ExprPtr b) { return make_nary("Mul", std::move(a), std::move(b), 1.0, 0.0); }

inline ExprPtr make_sub(ExprPtr a, ExprPtr b) {
    auto *na = as_num(a), *nb = as_num(b);
    if (na && nb) return make_num(na->value - nb->value);
    if (is_num(b, 0)) return a;
    if (a.get() == b.get()) return make_num(0);
    return make_add(a, make_mul(make_num(-1), b));
}

inline ExprPtr make_div(ExprPtr a, ExprPtr b) {
    auto *na = as_num(a), *nb = as_num(b);
    if (na && nb && nb->value != 0) return make_num(na->value / nb->value);
    if (is_num(a, 0)) return make_num(0);
    if (is_num(b, 1)) return a;
    if (a.get() == b.get()) return make_num(1);
    return make_mul(a, make_pow(b, make_num(-1)));
}

inline ExprPtr make_pow(ExprPtr a, ExprPtr b) {
    auto *na = as_num(a), *nb = as_num(b);
    if (na && nb) return make_num(std::pow(na->value, nb->value));
    if (is_num(b, 0)) return make_num(1);
    if (is_num(b, 1)) return a;
    if (is_num(a, 1)) return make_num(1);
    return make_apply("Pow", {std::move(a), std::move(b)});
}

inline ExprPtr make_log(ExprPtr a) {
    if (auto* n = as_num(a)) { if (n->value > 0) return make_num(std::log(n->value)); }
    if (auto* e = as_apply(a, "Exp")) return e->args[0];
    return make_apply("Log", {std::move(a)});
}

inline ExprPtr make_exp(ExprPtr a) {
    if (auto* n = as_num(a)) return make_num(std::exp(n->value));
    if (auto* l = as_apply(a, "Log")) return l->args[0];
    return make_apply("Exp", {std::move(a)});
}

inline ExprPtr make_derivative(ExprPtr target, const std::string& var, int order) {
    if (order == 0) return target;
    if (as_num(target)) return make_num(0);
    if (auto* d = as_apply(target, "Deriv"))
        if (d->deriv_var == var) return make_derivative(d->args[0], var, d->deriv_order + order);
    return make_apply("Deriv", {std::move(target)}, var, order);
}

inline ExprPtr Apply::derivative(const std::string& var) const {
    if (head == "Add") {
        ExprPtr r = args[0]->derivative(var);
        for (size_t i = 1; i < args.size(); ++i) r = make_add(r, args[i]->derivative(var));
        return r;
    }
    if (head == "Mul") { // N-ary Leibniz
        ExprPtr r = make_num(0);
        for (size_t i = 0; i < args.size(); ++i) {
            ExprPtr term = args[i]->derivative(var);
            for (size_t j = 0; j < args.size(); ++j)
                if (j != i) term = make_mul(term, args[j]);
            r = make_add(r, term);
        }
        return r;
    }
    if (head == "Pow") {
        auto* n = as_num(args[1]);
        if (n) return make_mul(make_mul(args[1], make_pow(args[0], make_num(n->value-1))), args[0]->derivative(var));
        ExprPtr uv = make_pow(args[0], args[1]);
        return make_mul(uv, make_add(make_mul(args[1]->derivative(var), make_log(args[0])),
                                     make_mul(args[1], make_div(args[0]->derivative(var), args[0]))));
    }
    if (head == "Log") return make_div(args[0]->derivative(var), args[0]);
    if (head == "Exp") return make_mul(make_exp(args[0]), args[0]->derivative(var));
    if (head == "Deriv") {
        if (var == deriv_var)
            return make_derivative(std::make_shared<const Apply>(head, args, deriv_var, deriv_order+1), var);
        throw std::runtime_error("Mixed partials not supported");
    }
    throw std::runtime_error("Unknown head in derivative: " + head);
}
