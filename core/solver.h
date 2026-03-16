#pragma once

#include "expr.h"
#include "integrator.h"
#include "ode_analyzer.h"
#include "ode_extractor.h"
#include "simplify.h"

#include <iostream>
#include <string>
#include <vector>

class LinearFirstOrderSolver {
public:
  static ExprPtr solve(const ExprPtr &equation, const std::string &indep_var,
                       const std::string &dep_var) {
    // 1. Extract P(x) and Q(x)
    StandardForm form =
        ODEExtractor::extract_linear_first_order(equation, indep_var, dep_var);

    // 2. Integrate P(x)
    ExprPtr int_P = Integrator::integrate(form.P, indep_var);

    // 3. Compute mu(x) = exp(int P(x) dx) and apply Transcendental Reduction
    ExprPtr mu = Simplifier::simplify(make_exp(int_P));

    // 4. Compute mu(x) * Q(x)
    ExprPtr mu_Q = Simplifier::simplify(make_mul(mu, form.Q));

    // 5. Integrate [mu(x) * Q(x)]
    ExprPtr int_mu_Q = Integrator::integrate(mu_Q, indep_var);

    // 6. Inject the Constant of Integration (C)
    ExprPtr C = make_sym("C"); // Represents arbitrary constant
    ExprPtr right_side = make_add(int_mu_Q, C);

    // 7. Final Assembly: y(x) = (1 / mu(x)) * [int_mu_Q + C]
    ExprPtr inv_mu = make_pow(mu, make_num(-1));
    ExprPtr solution = Simplifier::simplify(make_mul(inv_mu, right_side));

    return solution;
  }
};