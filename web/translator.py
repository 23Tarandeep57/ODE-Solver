"""
Stack-based LaTeX to CAS parser.
Converts LaTeX markup from vision models into linear algebraic syntax
that the C++ Pratt parser can consume.
"""


def find_matching_brace(s: str, start_index: int) -> int:
    """
    Computes the exact index of the matching '}' for a given '{'.
    O(N) traversal using a depth counter.
    """
    if s[start_index] != '{':
        return -1
    
    depth = 0
    for i in range(start_index, len(s)):
        if s[i] == '{':
            depth += 1
        elif s[i] == '}':
            depth -= 1
            if depth == 0:
                return i
    raise ValueError(f"Engine Fault: Unbalanced braces starting at index {start_index}")


def resolve_fractions_and_powers(latex_str: str) -> str:
    """
    Recursively collapses LaTeX \\frac{numerator}{denominator} into (numerator)/(denominator)
    and base^{exponent} into base^(exponent).
    """
    s = latex_str
    
    # 1. Resolve Fractions
    while "\\frac" in s:
        idx = s.find("\\frac")
        
        # Find Numerator bounds
        num_start = s.find("{", idx)
        num_end = find_matching_brace(s, num_start)
        numerator = s[num_start + 1 : num_end]
        
        # Find Denominator bounds
        den_start = s.find("{", num_end + 1)
        den_end = find_matching_brace(s, den_start)
        denominator = s[den_start + 1 : den_end]
        
        # Reconstruct string with strict parentheses
        replacement = f"({numerator})/({denominator})"
        s = s[:idx] + replacement + s[den_end + 1:]

    # 2. Resolve Superscripts (Powers)
    while "^{" in s:
        idx = s.find("^{")
        exp_start = idx + 1
        exp_end = find_matching_brace(s, exp_start)
        exponent = s[exp_start + 1 : exp_end]
        
        replacement = f"^({exponent})"
        s = s[:idx] + replacement + s[exp_end + 1:]

    return s


def translate_latex_to_cas(latex_str: str) -> str:
    """Master translation pipeline: LaTeX -> CAS algebra string."""
    # 0. Clean input formatting
    s = latex_str.replace(r"\left", "").replace(r"\right", "")
    s = s.replace(" ", "")
    
    # 1. Flatten the Context-Free Grammar using the stack algorithm
    s = resolve_fractions_and_powers(s)
    
    # 2. Map Multiplicative Operators
    s = s.replace(r"\cdot", "*").replace(r"\times", "*")
    
    # 3. Map Differential Operators
    s = s.replace("(dy)/(dx)", "y'")
    s = s.replace("(d^2y)/(dx^2)", "y''")
    s = s.replace("(d)/(dx)y", "y'") 
    s = s.replace("(d)/(dx)(y)", "y'") 
    
    # 4. Map Transcendental Functions
    s = s.replace(r"\sin", "sin")
    s = s.replace(r"\cos", "cos")
    s = s.replace(r"\ln", "ln")
    s = s.replace(r"\exp", "exp")
    
    # 5. Clean remaining structural braces
    s = s.replace("{", "(").replace("}", ")")
    
    # 6. Safety: strip all remaining backslashes
    s = s.replace("\\", "")
    return s
