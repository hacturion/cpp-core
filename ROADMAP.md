# mylib — Numerical Methods Library Roadmap

**Goal**: A high-quality, high-performance C++ numerical library that can be used seamlessly from both **Python (Jupyter)** and **Excel**, with particular strength in quantitative finance and non-standard valuation problems.

## Core Philosophy

- One implementation. Two (or more) excellent frontends.
- Numerical care matters (compensated summation, careful floating point, interval arithmetic later).
- Designed to host both new code and gradually modernized legacy code.
- Pragmatic: start simple, add sophistication where it delivers value.

## Major Capability Areas (Long Term)

### 1. Random Number Generation & Quasi-Monte Carlo
- High-quality uniform PRNGs (multiple algorithms)
- **Sobol** sequences (and other low-discrepancy sequences)
- Brownian motion / bridge constructions
- Antithetic, control variates, importance sampling infrastructure
- Reproducibility and seeding strategy across Python + Excel

### 2. Derivative Pricing Framework
- Generic Monte Carlo engine
- Path generation
- Payoff evaluation
- Greeks (pathwise, likelihood ratio, adjoint, finite difference)
- Variance reduction techniques
- Support for the user's own pricing algorithms

### 3. Life Settlement & Longevity Products
- Mortality / survival curve modeling
- Stochastic mortality models
- Cash flow projection engine under multiple scenarios
- Portfolio-level aggregation and risk metrics
- Regulatory / accounting views (US GAAP, IFRS, etc.)

### 4. Advanced Numerical Techniques
- **Interval arithmetic** (for rigorous error bounds and solver validation)
- Robust root finding and optimization with interval methods
- Automatic differentiation (forward + reverse) — extremely valuable for Greeks
- Stable linear algebra helpers (possibly on top of Eigen)

### 5. General Numerical / Math Utilities
- Accurate reductions (Kahan, pairwise, etc.)
- Special functions
- Interpolation (1D and later multi-D)
- Numerical integration (quadrature)
- Matrix utilities

## Current Status (as of this writing)

**Foundation + Random + Numerical Primitives phase** (current)

- Stable C ABI + modern pybind11 + NumPy
- CMake + Eigen support + VSCode configuration
- High-quality **Sobol** generator (Joe & Kuo direction numbers, up to 40D)
- Basic PRNG wrapper
- Basic **Interval** arithmetic skeleton
- First Monte Carlo demo (European call) using both PRNG and Sobol
- Kahan summation as the model for numerically careful reductions

## Near-Term Priorities (Next 4–8 Weeks)

1. **Solid vector/array foundation**
   - Excellent 1D array support (done for simple case)
   - 2D / matrix support
   - Optional: begin using Eigen

2. **Random number infrastructure**
   - At minimum: good Mersenne Twister + Sobol (even a basic Sobol is valuable)
   - Design the RNG interface so it can be used from both Python and Excel cleanly

3. **First real pricing example**
   - Something the user cares about (even a simplified version of one of their algorithms)
   - Demonstrate end-to-end: C++ → Python notebook + Excel UDF

4. **Life settlement primitives**
   - Mortality table loading
   - Basic survival probability / life expectancy calculations

5. **Developer experience**
   - Good Python package structure
   - Documentation + examples
   - Testing strategy (both numerical tests and regression tests)

## Open Design Questions

- How heavily do you want to rely on **Eigen** vs pure standard library?
- Do you want **automatic differentiation** early or later?
- For Sobol: do you want a full high-quality implementation, or is a "good enough" version + ability to load pre-generated sequences acceptable initially?
- Interval arithmetic: do you have a preferred flavor/implementation, or should we evaluate options?
- How important is **GPU / parallel** execution in the medium term?

## Non-Goals (at least initially)

- Becoming a full QuantLib replacement
- Heavy GUI / visualization (leave that to the frontends)
- Supporting every possible exotic under the sun

---

This document will evolve. The most valuable thing right now is to keep the foundation clean while we bring in real algorithms from your legacy work and new development.
