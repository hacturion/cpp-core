"""
Modern Python usage via pybind11 bindings.

This is the recommended way for numerical work because you get:
- Proper NumPy array support (zero-copy where possible)
- Natural Python exceptions
- Good docstrings in help()
- Python classes for stateful objects

After building with CMake (MYLIB_BUILD_PYTHON=ON), you should be able to do:

    import mylib
"""

import numpy as np

# This will only work after you have built the pybind11 module
try:
    import mylib
except ImportError as e:
    print("Could not import 'mylib'.")
    print("You need to build the Python bindings first using CMake + pybind11.")
    print("See BUILD_INSTRUCTIONS.md (updated section on CMake).")
    raise

print("mylib version:", mylib.__version__)

# --- Basic usage ---
print("\nBasic math:")
print("  add(2.5, 3.5) =", mylib.add(2.5, 3.5))

# --- NumPy integration (the important part for numerical methods) ---
print("\nNumPy integration (using Kahan summation):")
x = np.array([1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0])
y = np.array([10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0])

print("  sum(x) =", mylib.sum(x))
print("  add_arrays(x, y) =", mylib.add_arrays(x, y))

# Weighted sum example (very common in derivative pricing)
weights = np.array([0.1, 0.2, 0.3, 0.2, 0.1, 0.05, 0.03, 0.01, 0.005, 0.005])
print("  weighted_sum(x, weights) =", mylib.weighted_sum(x, weights))

ma = mylib.moving_average(x, window=3)
print("  moving_average(x, 3) =", ma)

mn, mx, mean, std = mylib.basic_stats(x)
print(f"  basic_stats(x) -> min={mn}, max={mx}, mean={mean}, std={std}")

# --- Stateful object ---
print("\nStateful context:")
ctx = mylib.create_context(100)
for v in [1.0, 3.0, 5.0, 7.0, 9.0]:
    ctx.add_value(v)
print("  Mean of added values =", ctx.mean())

print("\nAll pybind11 examples succeeded.")
