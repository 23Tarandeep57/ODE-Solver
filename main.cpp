#include <iostream>
#include <string>

#include "expr.h"
#include "ode_analyzer.h"
#include "ode_extractor.h"
#include "simplify.h"
#include "solver.h"
#include "tests.cpp"

using namespace std;

int main(int argc, char *argv[]) {
  // Run legacy tests if --test flag is passed
  if (argc > 1 && string(argv[1]) == "--test") {
    return run_tests();
  }

  try {
    // x * y' - 3*y - x^2 = 0
    ExprPtr x = make_sym("x");
    ExprPtr y = make_sym("y", "x"); // y depends on x
    ExprPtr y_prime = make_derivative(y, "x", 1);

    ExprPtr term1 = make_mul(x, y_prime);                             // x * y'
    ExprPtr term2 = make_mul(make_num(-3), y);                        // -3 * y
    ExprPtr term3 = make_mul(make_num(-1), make_pow(x, make_num(2))); // -x^2

    // F(x, y, y') = (x*y') + (-3*y) + (-x^2)
    ExprPtr raw_ode = make_add(make_add(term1, term2), term3);

    cout << "Input ODE: " << raw_ode->to_string() << " = 0\n";

    ExprPtr expanded_ode = Simplifier::expand_derivative(raw_ode);
    ExprPtr canonical_ode = Simplifier::simplify(expanded_ode);
    ODEMetrics metrics = ODEAnalyzer::analyze(canonical_ode, "y");

    cout << "Order : " << metrics.order << "\n";
    cout << "Degree : " << metrics.degree << "\n";
    cout << "Is Linear : " << (metrics.is_linear ? "True" : "False") << endl;

    // Routing & Extraction
    if (metrics.order == 1 && metrics.degree == 1 && metrics.is_linear) {
      cout << "\n[Routing to First-Order Linear Expert]\n";

      StandardForm form =
          ODEExtractor::extract_linear_first_order(canonical_ode, "x", "y");

      cout << "Standard Form: y' + P(x)y = Q(x)\n";
      cout << "P(x) : " << form.P->to_string() << "\n";
      cout << "Q(x) : " << form.Q->to_string() << "\n";
    } else {
      cout << "\n[Routing Error: Equation is not a First-Order Linear ODE.]\n";
    }

    cout << "\n[Solving ODE]\n";
    ExprPtr solution = LinearFirstOrderSolver::solve(canonical_ode, "x", "y");
    cout << "y(x) = " << solution->to_string() << "\n";

    cout << "\n[Testing reduce_exp Logic]\n";
    ExprPtr log_term = make_log(x);
    ExprPtr mul_term = make_mul(make_num(3), log_term); // 3 * ln(x)
    ExprPtr exp_term = make_exp(mul_term);              // exp(3 * ln(x))
    cout << "Constructed: exp(3 * ln(x))\n";
    cout << "Simplified : " << exp_term->to_string() << "\n";

  } catch (const exception &e) {
    cerr << "Engine Fault: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
