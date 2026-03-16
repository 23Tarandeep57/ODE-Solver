#pragma once

#include <iostream>
#include <map>
#include <string>

#include "ode_analyzer.h"
#include "parser.h"
#include "simplify.h"
using namespace std;

void print_step(const string &label, ExprPtr e) {
  ExprPtr simplified = Simplifier::simplify(e);
  cout << label << " : " << simplified->to_string() << endl;
}

ExprPtr ast_builder(string &input) {
  Lexer lex(input);
  Parser parser(lex);
  ExprPtr raw_ast = parser.parse_expression(0);
  ExprPtr expanded_ast = Simplifier::expand_derivative(raw_ast);
  ExprPtr final_ast = Simplifier::simplify(expanded_ast);
  return final_ast;
}

void print_ode_metrics(const string &label, const ODEMetrics &m) {
  cout << label << "\n";
  if (m.order == -1)
    cout << "not an ODE (dependent variable absent)\n";
  else {
    cout << "Order  : " << m.order << "\n";
    cout << "Degree : " << m.degree << "\n";
    cout << "Linear : " << (m.is_linear ? "Yes" : "No") << "\n";
  }
  cout << endl;
}

int run_tests() {
  try {
    map<string, double> env = {{"x", 4.0}};
    {
      cout << "Expression Swell Test (x * x) \n";
      string input = "x * x";
      ExprPtr f = ast_builder(input);
      cout << "f(x)    : " << f->to_string() << "\n";
      ExprPtr d1 = f->derivative("x");
      cout << "f'(x)   : " << d1->to_string() << "\n";
      ExprPtr d2 = d1->derivative("x");
      cout << "f''(x)  : " << d2->to_string() << "\n";
      ExprPtr d3 = d2->derivative("x");
      cout << "f'''(x) : " << d3->to_string() << "\n\n";
    }

    {
      cout << "implicit multiplication test (2x^2)\n";

      string input = "2x^2 + 5x";
      ExprPtr f = ast_builder(input);

      cout << "Input string is 2x^2 + 5x\n";
      print_step("parsed and simplified", f);

      ExprPtr d = f->derivative("x");
      print_step("d/dx", d);
      cout << endl << endl;
    }

    {
      cout << "Explicit Differentiation (y as function of x)\n";

      ExprPtr x = make_sym("x");
      ExprPtr y = make_sym("y", "x"); // y is dependednt on x

      ExprPtr f = make_mul(x, y);
      cout << "f(x,y) : " << f->to_string() << "\n";

      ExprPtr df_dx = f->derivative("x");
      print_step("d/dx (x,y) ", df_dx);
      cout << endl << endl;
    }

    {
      cout << "Polynomial: 3x^5 + 2x^3 - x \n";
      string input = "3 * x^5 + 2*x^3 - x";
      ExprPtr f = ast_builder(input);
      cout << "f(x) : " << f->to_string() << "\n";

      ExprPtr d = f;
      for (int i = 1; i <= 6; ++i) {
        d = d->derivative("x");
        cout << "d" << i << "/dx" << i << " : " << d->to_string() << "\n";
      }
      cout << "d5/dx5(x=4): "
           << f->derivative("x")
                  ->derivative("x")
                  ->derivative("x")
                  ->derivative("x")
                  ->derivative("x")
                  ->evaluate(env)
           << "\n\n";
    }

    {
      cout << "Generalized Power Rule: x^x \n";
      string input = "x^x";
      ExprPtr f = ast_builder(input);

      cout << "f(x)  : " << f->to_string() << "\n";
      ExprPtr d1 = f->derivative("x");
      cout << "f'(x) : " << d1->to_string() << "\n";
      cout << "f'(4) : " << d1->evaluate(env) << "\n\n";
    }

    {
      cout << "Identity collapse: exp(ln(x)) and exp(c * ln(x))\n";

      string input1 = "exp(ln(x^2))";
      ExprPtr f1 = ast_builder(input1);
      cout << "original   : exp(ln(x^2))\n";
      cout << "Simplified : " << f1->to_string() << "\n";

      string input2 = "exp(3 * ln(x))";
      ExprPtr f2 = ast_builder(input2);
      cout << "original   : exp(3 * ln(x))\n";
      cout << "Simplified : " << f2->to_string() << "\n";

      string input3 = "exp(-1 * ln(x))";
      ExprPtr f3 = ast_builder(input3);
      cout << "original   : exp(-1 * ln(x))\n";
      cout << "Simplified : " << f3->to_string() << "\n";

      cout << endl;
    }

    {
      cout << "Transcendental: exp(2*x) + ln(x) \n";
      string input = "exp(2*x) + ln(x)";
      ExprPtr f = ast_builder(input);

      cout << "f(x) : " << f->to_string() << "\n";
      cout << "f(4) : " << f->evaluate(env) << "\n";

      ExprPtr d1 = f->derivative("x");
      cout << "f'(x) : " << d1->to_string() << "\n";
      cout << "f'(4) : " << d1->evaluate(env) << "\n";

      ExprPtr d2 = d1->derivative("x");
      cout << "f''(x) : " << d2->to_string() << "\n";
      cout << "f''(4) : " << d2->evaluate(env) << "\n\n";
    }

    {
      cout << "DAG Identity (hash consing) \n";
      ExprPtr x1 = make_sym("x");
      ExprPtr x2 = make_sym("x");
      cout << "make_sym(\"x\") called twice -> same pointer? "
           << (x1.get() == x2.get() ? "YES" : "NO") << "\n";

      ExprPtr n1 = make_num(42);
      ExprPtr n2 = make_num(42);
      cout << "make_num(42) called twice -> same pointer? "
           << (n1.get() == n2.get() ? "YES" : "NO") << "\n";

      ExprPtr a1 = make_add(x1, n1);
      ExprPtr a2 = make_add(x2, n2);
      cout << "make_add(x,42) twice -> same pointer? "
           << (a1.get() == a2.get() ? "YES" : "NO") << "\n\n";

      ExprPtr m1 = make_mul(x1, n1);
      ExprPtr m2 = make_mul(n2, x2);
      cout << "42*x vs x*42 twice -> same pointer? "
           << (m1.get() == m2.get() ? "YES" : "NO") << "\n\n";
    }

    {
      ExprPtr x = make_sym("x");
      ExprPtr y = make_sym("y", "x");
      ExprPtr dy = make_derivative(y, "x", 1);  // y'
      ExprPtr d2y = make_derivative(y, "x", 2); // y''

      // 1) y' + y = x  =>  y' + y - x  (first-order, degree 1, linear)
      {
        ExprPtr ode = make_sub(make_add(dy, y), x);
        ODEMetrics m = ODEAnalyzer::analyze(ode, "y");
        print_ode_metrics("y' + y - x  (expect: order=1, degree=1, linear)", m);
      }

      // 2) y'' + y' + y = 0  (second-order, degree 1, linear)
      {
        ExprPtr ode = make_add(d2y, make_add(dy, y));
        ODEMetrics m = ODEAnalyzer::analyze(ode, "y");
        print_ode_metrics("y'' + y' + y  (expect: order=2, degree=1, linear)",
                          m);
      }

      // 3) (y')^2 + y = 0  (first-order, degree 2, non-linear)
      {
        ExprPtr ode = make_add(make_pow(dy, make_num(2)), y);
        ODEMetrics m = ODEAnalyzer::analyze(ode, "y");
        print_ode_metrics("(y')^2 + y  (expect: order=1, degree=2, non-linear)",
                          m);
      }

      // 4) y * y' = x  =>  y*y' - x  (first-order, degree 1, non-linear due to
      // product)
      {
        ExprPtr ode = make_sub(make_mul(y, dy), x);
        ODEMetrics m = ODEAnalyzer::analyze(ode, "y");
        print_ode_metrics("y*y' - x  (expect: order=1, degree=1, non-linear)",
                          m);
      }

      // 5) y'' + x*y' + y = 0  (second-order, degree 1, linear)
      {
        ExprPtr ode = make_add(d2y, make_add(make_mul(x, dy), y));
        ODEMetrics m = ODEAnalyzer::analyze(ode, "y");
        print_ode_metrics("y'' + x*y' + y  (expect: order=2, degree=1, linear)",
                          m);
      }

      // 6) (y'')^3 + y' = 0  (second-order, degree 3, non-linear)
      {
        ExprPtr ode = make_add(make_pow(d2y, make_num(3)), dy);
        ODEMetrics m = ODEAnalyzer::analyze(ode, "y");
        print_ode_metrics(
            "(y'')^3 + y'  (expect: order=2, degree=3, non-linear)", m);
      }

      // 7) Pure algebraic, no y: x^2 + 1
      {
        ExprPtr expr = make_add(make_pow(x, make_num(2)), make_num(1));
        ODEMetrics m = ODEAnalyzer::analyze(expr, "y");
        print_ode_metrics("x^2 + 1  (expect: not an ODE)", m);
      }

      // 8) exp(y) + y' = 0  (first-order, degree 1, non-linear due to exp(y))
      {
        ExprPtr ode = make_add(make_exp(y), dy);
        ODEMetrics m = ODEAnalyzer::analyze(ode, "y");
        print_ode_metrics(
            "exp(y) + y'  (expect: order=1, degree=1, non-linear)", m);
      }
    }

  } catch (const exception &e) {
    cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
