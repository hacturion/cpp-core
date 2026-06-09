# C++ Core Library for Numerical Methods (Python + Excel)

This is a modern skeleton for a **numerical methods library** written in C++ that you want to call from **both Python notebooks and Excel**.

## Why This Architecture?

Numerical methods (Monte Carlo, PDE solvers, optimization, interpolation, stochastic processes, custom linear algebra, etc.) are expensive to implement and maintain. You don't want two versions.

This setup gives you:

- High-performance C++ as the single source of truth
- Excellent NumPy integration from Python (via **pybind11**)
- Excel UDFs via **xlwings** (recommended) or direct calls from Python notebooks
- A clean path for bringing in / modernizing legacy C++ code

## Project Structure

```
cpp-core/
├── include/           # Public C++ headers
├── src/               # Core implementation
├── python/            # pybind11 bindings
├── examples/
│   ├── cpp/                  # Standalone C++ examples (Monte Carlo, etc.)
│   └── python/               # Optional Python examples (disabled by default)
│       ├── use_from_python.py
│       └── use_from_python_pybind11.py
├── CMakeLists.txt
└── BUILD_INSTRUCTIONS.md
```

The primary (and only officially supported in this configuration) way to use this library from Excel is via the **native XLL** in the sibling `excel-udf/` project.

This gives you:
- All heavy numerical work in **pure C++** (`mylib.dll`)
- Easy UDF registration and packaging via Excel-DNA (thin C# layer)
- No Python dependency at runtime

Python bindings are disabled by default.

## Recommended Tooling (Given You Use VSCode)

- **CMake** + **Visual Studio 2022 Build Tools** (MSVC compiler)
- VSCode + **CMake Tools** + **C/C++** extensions
- pybind11 (fetched automatically by CMake)
- (Strongly recommended later) **Eigen** for linear algebra

See [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md) for the current recommended CMake flow.

## Current State

We have a small working example with:

- Basic numerical helpers (sum, moving average, stats, etc.)
- Both a raw C ABI (for Excel / ctypes) **and** modern pybind11 bindings
- Working P/Invoke from the Excel-DNA project

This is scaffolding that is now being shaped around real numerical finance work (Monte Carlo / quasi-Monte Carlo, derivative pricing, life settlements, interval arithmetic, high-quality random numbers including Sobol, etc.).

See [ROADMAP.md](ROADMAP.md) for the longer-term vision.

## Next Steps — How Would You Like to Proceed?

1. **Help me understand your numerical domain** (Monte Carlo simulation? Finite differences? Optimization? Calibration? Something else?)
2. **Bring in some legacy code** — We can start wrapping specific functions or classes
3. **Set up a proper project layout** for a larger numerical library (modules, namespaces, testing, etc.)
4. **Improve array/matrix support** (2D, column-major, Eigen integration, etc.)
5. **Python packaging story** (so you can `pip install` your library later)
6. **Excel-side ergonomics** for large numerical arrays/ranges

Just tell me where you want to go first. I'm happy to iterate on the structure, add real numerical examples, or help wrap pieces of your existing code.
