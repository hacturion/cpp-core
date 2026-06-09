"""
Example: Calling the C++ mylib.dll from Python using ctypes.

This works with a plain Python install (no pybind11 required).
As soon as you have real Python + the compiled mylib.dll, this should just work.
"""

import ctypes
import os
import sys
from ctypes import c_double, c_int, POINTER, c_char_p

# ------------------------------------------------------------------
# Locate the DLL
# ------------------------------------------------------------------
# Put mylib.dll next to this script, or in the current working directory,
# or add its folder to PATH / LD_LIBRARY_PATH equivalent on Windows.
DLL_NAME = "mylib.dll"

# Try common locations
script_dir = os.path.dirname(os.path.abspath(__file__))
candidates = [
    os.path.join(script_dir, DLL_NAME),
    os.path.join(script_dir, "..", "..", "build", DLL_NAME),           # from examples/python
    os.path.join(script_dir, "..", "..", "..", "build", DLL_NAME),     # deeper nesting
    os.path.join(os.getcwd(), DLL_NAME),
    DLL_NAME,  # rely on PATH
]

dll_path = None
for cand in candidates:
    if os.path.exists(cand):
        dll_path = os.path.abspath(cand)
        break

if dll_path is None:
    print("ERROR: Could not find mylib.dll")
    print("Looked in:", candidates)
    print("\nPlease compile the DLL first (see BUILD_INSTRUCTIONS.md)")
    sys.exit(1)

print(f"Loading C++ library from: {dll_path}")
lib = ctypes.CDLL(dll_path)

# ------------------------------------------------------------------
# Declare function signatures (very important for correctness)
# ------------------------------------------------------------------
lib.mylib_version.restype = c_char_p

lib.mylib_add.argtypes = [c_double, c_double]
lib.mylib_add.restype = c_double

lib.mylib_multiply.argtypes = [c_double, c_double]
lib.mylib_multiply.restype = c_double

lib.mylib_sum.argtypes = [POINTER(c_double), c_int]
lib.mylib_sum.restype = c_double

# For array input + output, we use POINTER(c_double)
DoubleArray = POINTER(c_double)

lib.mylib_add_arrays.argtypes = [DoubleArray, DoubleArray, DoubleArray, c_int]
lib.mylib_add_arrays.restype = None

lib.mylib_moving_average.argtypes = [DoubleArray, DoubleArray, c_int, c_int]
lib.mylib_moving_average.restype = None

lib.mylib_basic_stats.argtypes = [DoubleArray, c_int, DoubleArray]
lib.mylib_basic_stats.restype = c_int

# Opaque handle functions
class MyLibHandle_(ctypes.Structure):
    pass

MyLibHandle = POINTER(MyLibHandle_)

lib.mylib_create_context.argtypes = [c_int]
lib.mylib_create_context.restype = MyLibHandle

lib.mylib_destroy_context.argtypes = [MyLibHandle]
lib.mylib_destroy_context.restype = None

lib.mylib_context_add_value.argtypes = [MyLibHandle, c_double]
lib.mylib_context_add_value.restype = c_int

lib.mylib_context_get_mean.argtypes = [MyLibHandle]
lib.mylib_context_get_mean.restype = c_double

# ------------------------------------------------------------------
# Helper to convert Python list → ctypes array
# ------------------------------------------------------------------
def to_c_array(py_list):
    arr = (c_double * len(py_list))(*py_list)
    return arr, len(py_list)

# ------------------------------------------------------------------
# Demo usage
# ------------------------------------------------------------------
def main():
    print("\n=== C++ Library called from Python (ctypes) ===\n")

    # Version
    version = lib.mylib_version().decode("utf-8")
    print(f"Library version: {version}\n")

    # Simple math (now backed by real C++ implementations)
    print("Basic math:")
    print(f"  mylib_add(3.5, 2.25)     = {lib.mylib_add(3.5, 2.25)}")
    print(f"  mylib_multiply(4, 7)     = {lib.mylib_multiply(4, 7)}")

    # Array operations
    print("\nArray operations:")
    a = [1.0, 2.0, 3.0, 4.0, 5.0]
    b = [10.0, 20.0, 30.0, 40.0, 50.0]

    a_arr, n = to_c_array(a)
    b_arr, _ = to_c_array(b)
    result_arr = (c_double * n)()

    lib.mylib_add_arrays(a_arr, b_arr, result_arr, n)
    result = [result_arr[i] for i in range(n)]
    print(f"  add_arrays(a, b)         = {result}")

    total = lib.mylib_sum(a_arr, n)
    print(f"  sum(a)                   = {total}")

    # Moving average
    print("\nMoving average (window=3):")
    input_data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    inp_arr, m = to_c_array(input_data)
    out_arr = (c_double * m)()

    lib.mylib_moving_average(inp_arr, out_arr, m, 3)
    ma = [round(out_arr[i], 4) for i in range(m)]
    print(f"  input                    = {input_data}")
    print(f"  moving_average (w=3)     = {ma}")

    # Stats
    print("\nBasic statistics:")
    stats = (c_double * 4)()
    rc = lib.mylib_basic_stats(a_arr, n, stats)
    if rc == 0:
        print(f"  min, max, mean, stddev   = {tuple(round(stats[i], 4) for i in range(4))}")

    # Opaque context / stateful object
    print("\nStateful context (opaque handle):")
    ctx = lib.mylib_create_context(10)
    for val in [10, 20, 30, 40, 50]:
        lib.mylib_context_add_value(ctx, val)

    mean = lib.mylib_context_get_mean(ctx)
    print(f"  Added values [10..50], mean = {mean}")

    lib.mylib_destroy_context(ctx)
    print("  Context destroyed.")

    print("\n=== All calls succeeded ===")


if __name__ == "__main__":
    main()
