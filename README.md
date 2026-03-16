---
title: ODE Solver
sdk: docker
app_port: 7860
---

# ODE Solver

A Computer Algebra System that solves first-order linear Ordinary Differential Equations analytically.

Upload an image of a handwritten or printed ODE, and the engine will:
1. **Extract** the equation using a Vision Transformer (Pix2Text)
2. **Translate** the LaTeX into algebraic syntax via a stack-based parser
3. **Solve** it analytically using a custom C++ symbolic engine
