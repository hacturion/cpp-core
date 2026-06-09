"""
xlwings User Defined Functions for Excel.

This is the **canonical** example for exposing the C++ mylib numerical library
(via pybind11) as native Excel UDFs.

Location: cpp-core/examples/python/xlwings/udfs.py
Recommended import location for other projects.

Usage:
1. Build cpp-core with CMake + MYLIB_BUILD_PYTHON=ON (produces mylib.pyd).
2. Make sure the compiled `mylib` module is importable (PYTHONPATH or copy .pyd).
3. Install xlwings: pip install xlwings
4. In Excel, go to the xlwings ribbon → Import Functions and select this file
   (or use xlwings CLI / add-in).

The other xlwings setups (NumericalTools/, mylib_xlwings/) can copy this file
or reference it.
"""

import numpy as np
import xlwings as xw

# Import the compiled C++ library (built with pybind11)
try:
    import mylib
except ImportError:
    mylib = None


# =============================================================================
# Helper
# =============================================================================
def _ensure_mylib():
    if mylib is None:
        raise RuntimeError(
            "The 'mylib' C++ extension could not be imported. "
            "Make sure you built cpp-core with MYLIB_BUILD_PYTHON=ON "
            "and that the resulting .pyd is on your Python path "
            "(see BUILD_INSTRUCTIONS.md)."
        )


# =============================================================================
# UDFs exposed to Excel
# =============================================================================

@xw.func
@xw.arg("values", np.array, ndim=1)
def my_sum(values: np.ndarray) -> float:
    """Sum using Kahan compensated summation from the C++ library (higher accuracy for large arrays)."""
    _ensure_mylib()
    return mylib.sum(values)


@xw.func
@xw.arg("values", np.array, ndim=1)
@xw.arg("weights", np.array, ndim=1)
def my_weighted_sum(values: np.ndarray, weights: np.ndarray) -> float:
    """Weighted sum using Kahan summation from C++. Great for discounted payoffs / MC."""
    _ensure_mylib()
    return mylib.weighted_sum(values, weights)


@xw.func
@xw.arg("a", float)
@xw.arg("b", float)
def my_add(a: float, b: float) -> float:
    """Simple add (C++ implementation, for parity with NATIVE.ADD)."""
    _ensure_mylib()
    return mylib.add(a, b)


@xw.func
@xw.arg("a", float)
@xw.arg("b", float)
def my_multiply(a: float, b: float) -> float:
    """Simple multiply (C++ implementation)."""
    _ensure_mylib()
    return mylib.multiply(a, b)


@xw.func
@xw.arg("dimension", int)
@xw.arg("n_points", int)
def my_sobol_points(dimension: int = 5, n_points: int = 10):
    """
    Generate the first n_points from a Sobol sequence (quasi-random, high quality for MC).
    Returns a 2D array (spills nicely in modern Excel).
    Example: =my_sobol_points(5, 20)
    """
    _ensure_mylib()
    dimension = int(dimension)
    n_points = int(n_points)
    sobol = mylib.random.make_sobol(dimension)
    points = [sobol.next() for _ in range(n_points)]
    return np.array(points)


@xw.func
@xw.arg("lower", float)
@xw.arg("upper", float)
def my_interval_example(lower: float, upper: float):
    """Simple demonstration of the Interval type from C++ (rigorous bounds)."""
    _ensure_mylib()
    iv = mylib.Interval(lower, upper)
    return f"[{iv.lower}, {iv.upper}], width={iv.width():.6f}"


# Optional: debug helper
@xw.func
def debug_mylib():
    """Debug: returns Python + mylib version info."""
    import sys
    try:
        import mylib
        return f"OK - Python: {sys.executable} - mylib: {getattr(mylib, '__version__', 'n/a')}"
    except Exception as e:
        return f"ERROR - Python: {sys.executable} - {type(e).__name__}: {e}"


@xw.func
@xw.arg("S0", float)
@xw.arg("K", float)
@xw.arg("r", float)
@xw.arg("sigma", float)
@xw.arg("T", float)
@xw.arg("n_paths", int)
@xw.arg("n_steps", int)
@xw.arg("use_sobol", bool)
def my_european_call_mc(S0: float, K: float, r: float, sigma: float, T: float,
                        n_paths: int = 10000, n_steps: int = 1, use_sobol: bool = False):
    """
    European call price via Monte Carlo using the C++ library (Sobol or PRNG + Kahan).
    Very basic toy version for demonstration.
    """
    _ensure_mylib()
    return mylib.european_call_mc(S0, K, r, sigma, T, int(n_paths), int(n_steps), bool(use_sobol))


# More functions can (and should) be added here as you expose more from the C++ library.
# See also the full example in examples/cpp/monte_carlo_european.cpp.
