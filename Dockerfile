# Stage 1: Compile C++ Engine
FROM gcc:13 AS builder
WORKDIR /build
COPY core/ ./
# -O3 for vectorization, -march=x86-64 for cloud compatibility
RUN g++ -std=c++17 -O3 -march=x86-64 -o ode_solver main.cpp -Wall -Wextra

# Stage 2: Python Runtime
FROM python:3.11-slim
WORKDIR /app

# 1. Install System Dependencies (OpenCV requirement)
# Updated for Debian Bookworm (Python 3.11-slim) compatibility
RUN apt-get update && apt-get install -y --no-install-recommends \
    libgl1 \
    libglib2.0-0 \
    && rm -rf /var/lib/apt/lists/*

# 2. Create non-root user (UID 1000) for HF Spaces security
RUN useradd -m -u 1000 appuser

# 3. Install Python dependencies (high-latency layer, cached)
COPY --chown=appuser:appuser requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# 4. Copy binary from builder
COPY --from=builder --chown=appuser:appuser /build/ode_solver /app/ode_solver
RUN chmod +x /app/ode_solver

# 5. Copy application code
COPY --chown=appuser:appuser web/ /app/web/
COPY --chown=appuser:appuser static/ /app/static/

# 6. Environment
ENV ODE_ENGINE_PATH=/app/ode_solver
ENV PYTHONPATH=/app

USER appuser
EXPOSE 7860

WORKDIR /app/web
CMD ["python", "server.py"]