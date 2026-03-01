#include <iostream>
#include <map>
#include <string>

#include "parser.h"
using namespace std;

int main() {
    try {
        map<string, double> env = {{"x", 4.0}};
        {
            cout << "Expression Swell Test (x * x) \n";
            string input = "x * x";
            Lexer lex(input);
            Parser parser(lex);
            ExprPtr f = parser.parse_expression();

            cout << "f(x)    : " << f->to_string() << "\n";
            ExprPtr d1 = f->derivative("x");
            cout << "f'(x)   : " << d1->to_string() << "\n";
            ExprPtr d2 = d1->derivative("x");
            cout << "f''(x)  : " << d2->to_string() << "\n";
            ExprPtr d3 = d2->derivative("x");
            cout << "f'''(x) : " << d3->to_string() << "\n\n";
        }

        {
            cout << "Polynomial: 3x^5 + 2x^3 - x \n";
            string input = "3 * x^5 + 2*x^3 - x";
            Lexer lex(input);
            Parser parser(lex);
            ExprPtr f = parser.parse_expression();
            cout << "f(x)       : " << f->to_string() << "\n";

            ExprPtr d = f;
            for (int i = 1; i <= 6; ++i) {
                d = d->derivative("x");
                cout << "d" << i << "/dx" << i << " : " << d->to_string() << "\n";
            }
            cout << "d5/dx5(x=4): " << f->derivative("x")
                            ->derivative("x")->derivative("x")
                            ->derivative("x")->derivative("x")
                            ->evaluate(env) << "\n\n";
        }

        {
            cout << "Generalized Power Rule: x^x \n";
            string input = "x^x";
            Lexer lex(input);
            Parser parser(lex);
            ExprPtr f = parser.parse_expression();

            cout << "f(x)  : " << f->to_string() << "\n";
            ExprPtr d1 = f->derivative("x");
            cout << "f'(x) : " << d1->to_string() << "\n";
            cout << "f'(4) : " << d1->evaluate(env) << "\n\n";
        }

        {
            cout << "Transcendental: exp(2*x) + ln(x) \n";
            string input = "exp(2*x) + ln(x)";
            Lexer lex(input);
            Parser parser(lex);
            ExprPtr f = parser.parse_expression();

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
            cout << "make_sym(\"x\") called twice —> same pointer? "
                      << (x1.get() == x2.get() ? "YES" : "NO") << "\n";

            ExprPtr n1 = make_num(42);
            ExprPtr n2 = make_num(42);
            cout << "make_num(42) called twice —> same pointer? "
                      << (n1.get() == n2.get() ? "YES" : "NO") << "\n";

            ExprPtr a1 = make_add(x1, n1);
            ExprPtr a2 = make_add(x2, n2);
            cout << "make_add(x,42) twice —> same pointer? "
                      << (a1.get() == a2.get() ? "YES" : "NO") << "\n\n";
        }

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
