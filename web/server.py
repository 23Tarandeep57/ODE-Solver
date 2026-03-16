"""
FastAPI application: ODE Solver web interface.
Handles image upload, LaTeX extraction, translation, and C++ engine execution.
"""
import io
import os
from pathlib import Path
from PIL import Image
from fastapi import FastAPI, UploadFile, File
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse

from translator import translate_latex_to_cas
from utils import execute_cpp_engine

app = FastAPI()

# Serve static frontend files from /static directory
STATIC_DIR = Path(__file__).parent.parent / "static"
app.mount("/static", StaticFiles(directory=str(STATIC_DIR)), name="static")

@app.get("/")
async def serve_index():
    return FileResponse(str(STATIC_DIR / "index.html"))

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# VRAM Lifecycle Management: Global Initialization
# Pins ResNet/Transformer weights into RAM/VRAM at server boot.
from pix2text import Pix2Text

print("[System] Loading P2T MFR Model...")
p2t_engine = Pix2Text(analyzer_config={'model_name': 'mfr'}) 
print("[System] Model active. Awaiting tensors.")

def extract_math_from_image(image_bytes: bytes) -> str:
    """Passes tensor to Pix2Text for handwriting-aware inference."""
    img = Image.open(io.BytesIO(image_bytes)).convert('RGB')
    result = p2t_engine.recognize_formula(img)
    return result

@app.post("/solve")
async def solve_equation(file: UploadFile = File(...)):
    image_bytes = await file.read()
    
    raw_latex = extract_math_from_image(image_bytes)
    print(f"[Vision] Extracted LaTeX: {raw_latex}")
    
    cas_string = translate_latex_to_cas(raw_latex)
    print(f"[Translator] Parsed String: {cas_string}")
    
    solution = execute_cpp_engine(cas_string)
    
    return {
        "latex": raw_latex, 
        "parsed_equation": cas_string, 
        "solution": solution
    }

if __name__ == "__main__":
    import uvicorn
    port = int(os.environ.get("PORT", 7860))
    uvicorn.run("server:app", host="0.0.0.0", port=port, reload=True)