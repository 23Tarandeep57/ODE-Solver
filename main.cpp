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

  if (argc < 2) {
    cout << "Usage: ./ode_solver \"<equation>\"\n       ./ode_solver --test\n";
    return 1;
  }

  try {
    string input_eq = argv[1];
    Lexer lex(input_eq);
    Parser parser(lex);
    ExprPtr raw_ode = parser.parse_expression(0);

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



  } catch (const exception &e) {
    cerr << "Engine Fault: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
