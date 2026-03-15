import io
import subprocess
from PIL import Image
from fastapi import FastAPI, UploadFile, File
from fastapi.middleware.cors import CORSMiddleware
# from pix2tex.cli import LatexOCR
import re

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# 1. VRAM Lifecycle Management: Global Initialization
# The model must be instantiated here, outside the routing function. 
# This pins the ResNet/Transformer weights into RAM/VRAM at server boot. 
# If you place this inside the @app.post route, you will trigger a catastrophic 
# disk I/O bottleneck, reloading hundreds of megabytes on every single API call.
from pix2text import Pix2Text

# Initialize globally to pin weights to VRAM
print("[System] Loading P2T MFR Model...")
p2t_engine = Pix2Text(analyzer_config={'model_name': 'mfr'}) 

def extract_math_from_image(image_bytes: bytes) -> str:
    """Passes tensor to Pix2Text for handwriting-aware inference."""
    # Write bytes to temporary memory or pass PIL Image directly based on P2T API
    img = Image.open(io.BytesIO(image_bytes)).convert('RGB')
    result = p2t_engine.recognize_formula(img)
    return result

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
    Recursively collapses LaTeX \frac{numerator}{denominator} into (numerator)/(denominator)
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
        # e.g., \frac{a}{b} -> (a)/(b)
        replacement = f"({numerator})/({denominator})"
        s = s[:idx] + replacement + s[den_end + 1:]

    # 2. Resolve Superscripts (Powers)
    while "^{" in s:
        idx = s.find("^{")
        exp_start = idx + 1
        exp_end = find_matching_brace(s, exp_start)
        exponent = s[exp_start + 1 : exp_end]
        
        # e.g., x^{2y} -> x^(2y)
        replacement = f"^({exponent})"
        s = s[:idx] + replacement + s[exp_end + 1:]

    return s

def translate_latex_to_cas(latex_str: str) -> str:
    # 0. Clean input formatting
    s = latex_str.replace(r"\left", "").replace(r"\right", "")
    s = s.replace(" ", "") # Strip all whitespace immediately
    
    # 1. Flatten the Context-Free Grammar using the stack algorithm
    s = resolve_fractions_and_powers(s)
    
    # 2. Map Multiplicative Operators
    s = s.replace(r"\cdot", "*").replace(r"\times", "*")
    
    # 3. Map Differential Operators (Now safe because \frac is resolved)
    # pix2tex often outputs dy and dx without slashes.
    # The stack parser converted \frac{dy}{dx} to (dy)/(dx).
    s = s.replace("(dy)/(dx)", "y'")
    s = s.replace("(d^2y)/(dx^2)", "y''") # For 2nd order ODEs
    
    # Handle the alternate notation \frac{d}{dx}y -> (d)/(dx)y
    s = s.replace("(d)/(dx)y", "y'") 
    s = s.replace("(d)/(dx)(y)", "y'") 
    
    # 4. Map Transcendental Functions
    s = s.replace(r"\sin", "sin")
    s = s.replace(r"\cos", "cos")
    s = s.replace(r"\ln", "ln")
    s = s.replace(r"\exp", "exp")
    
    # 5. Clean remaining structural braces (if any single-character exponents remain like x^2)
    s = s.replace("{", "(").replace("}", ")")
    
    # 6. Safety check: Strip all remaining backslashes to protect the C++ Parser 
    s = s.replace("\\", "")
    return s

def execute_cpp_engine(equation_str: str) -> str:
    try:
        result = subprocess.run(
            ["../ode_solver", equation_str], 
            capture_output=True, 
            text=True, 
            check=True
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        return f"C++ Engine Fault: {e.stderr}"

@app.post("/solve")
async def solve_equation(file: UploadFile = File(...)):
    image_bytes = await file.read()
    
    raw_latex = extract_math_from_image(image_bytes)
    print(f"[Vision] Extracted LaTeX: {raw_latex}")
    
    cas_string = translate_latex_to_cas(raw_latex)
    cas_string = "x*y' - 3*y - x^2 = 0"  # HARDCODED: bypass vision model for C++ engine testing
    print(f"[Translator] Parsed String: {cas_string}")
    
    # 3. Analytical Engine -> Solution
    solution = execute_cpp_engine(cas_string)
    
    return {
        "latex": raw_latex, 
        "parsed_equation": cas_string, 
        "solution": solution
    }

if __name__ == "__main__":
    import uvicorn
    # Pass 'server:app' as a string and enable reload=True so changes take effect immediately
    uvicorn.run("server:app", host="127.0.0.1", port=8000, reload=True)