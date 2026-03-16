"""
Helper functions for subprocess calls to the C++ ODE engine.
"""
import os
import subprocess
from pathlib import Path

# Resolve engine path: Docker uses /app/ode_solver, local dev uses ../ode_solver
ENGINE_PATH = os.environ.get(
    "ODE_ENGINE_PATH",
    str(Path(__file__).parent.parent / "ode_solver")
)


def execute_cpp_engine(equation_str: str) -> str:
    """Forks the process to execute the C++ engine with the given equation."""
    try:
        result = subprocess.run(
            [ENGINE_PATH, equation_str],
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        return f"C++ Engine Fault: {e.stderr}"
