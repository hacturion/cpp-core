"""
Cross-environment verification for mylib.

Works with:
- pybind11 .pyd (when PYTHONPATH points to a matching build, e.g. Python 3.11 .pyd)
- ctypes + mylib.dll (works with any Python, including uv 3.12)

Usage examples:
    # With pybind (3.11)
    $env:PYTHONPATH = "build\Release"
    python examples/python/verify_mylib.py

    # With ctypes (any Python)
    python examples/python/verify_mylib.py --ctypes
"""

import argparse
import os
import sys
from pathlib import Path

import numpy as np

def find_dll():
    candidates = [
        Path.cwd() / "mylib.dll",
        Path(__file__).parent.parent.parent / "build" / "Release" / "mylib.dll",
        Path(__file__).parent.parent.parent / "build" / "mylib.dll",
        Path(os.environ.get("PYTHONPATH", "")) / "mylib.dll",
    ]
    for c in candidates:
        if c.exists():
            return str(c.resolve())
    return None


def test_via_pybind():
    print("=== Testing via pybind11 (import mylib) ===")
    try:
        import mylib
    except ImportError as e:
        print(f"  SKIPPED: could not import mylib ({e})")
        return False

    print(f"  mylib version: {getattr(mylib, '__version__', 'n/a')}")
    print(f"  available: {[x for x in dir(mylib) if not x.startswith('_')]}")

    x = np.array([1.0, 2.0, 3.0, 4.0, 5.0, 100.0], dtype=np.float64)
    print(f"  Kahan sum(x)     = {mylib.sum(x)}")
    print(f"  basic_stats      = {mylib.basic_stats(x)}")
    print(f"  moving_average   = {mylib.moving_average(x, 3)[:4]}")

    if hasattr(mylib, "add") and hasattr(mylib, "multiply"):
        print(f"  mylib.add(2,3)   = {mylib.add(2, 3)}")
        print(f"  mylib.multiply(4,5) = {mylib.multiply(4, 5)}")

    if hasattr(mylib, "random"):
        s = mylib.random.make_sobol(4)
        print(f"  Sobol 4d sample  = {s.next()}")

    if hasattr(mylib, "Interval"):
        iv = mylib.Interval(1.0, 5.5)
        print(f"  Interval example = {iv}, width={iv.width()}")

    print("  pybind11 path: OK\n")
    return True


def test_via_ctypes(dll_path: str | None = None):
    print("=== Testing via ctypes (raw DLL) ===")
    import ctypes
    from ctypes import c_double, c_int, POINTER, c_char_p

    if dll_path is None:
        dll_path = find_dll()
    if not dll_path or not Path(dll_path).exists():
        print(f"  SKIPPED: could not find mylib.dll (looked in common places)")
        return False

    print(f"  Loading: {dll_path}")
    lib = ctypes.CDLL(dll_path)

    lib.mylib_version.restype = c_char_p

    # New ABI functions (we added these)
    lib.mylib_add.argtypes = [c_double, c_double]
    lib.mylib_add.restype = c_double
    lib.mylib_multiply.argtypes = [c_double, c_double]
    lib.mylib_multiply.restype = c_double

    lib.mylib_sum.argtypes = [POINTER(c_double), c_int]
    lib.mylib_sum.restype = c_double

    lib.mylib_basic_stats.argtypes = [POINTER(c_double), c_int, POINTER(c_double)]
    lib.mylib_basic_stats.restype = c_int

    print(f"  version: {lib.mylib_version().decode('utf-8', errors='ignore')}")

    data = [1.0, 2.0, 3.0, 4.0, 5.0, 100.0]
    arr = (c_double * len(data))(*data)
    print(f"  ctypes sum       = {lib.mylib_sum(arr, len(data))}")

    stats = (c_double * 4)()
    lib.mylib_basic_stats(arr, len(data), stats)
    print(f"  ctypes stats     = min={stats[0]:.2f}, max={stats[1]:.2f}, mean={stats[2]:.2f}, std={stats[3]:.2f}")

    print(f"  mylib_add(2,3)   = {lib.mylib_add(2, 3)}")
    print(f"  mylib_multiply(4,5) = {lib.mylib_multiply(4, 5)}")

    print("  ctypes path: OK\n")
    return True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--ctypes", action="store_true", help="Force ctypes path")
    parser.add_argument("--dll", type=str, default=None, help="Explicit path to mylib.dll for ctypes")
    args = parser.parse_args()

    print(f"Python: {sys.executable} ({sys.version.split()[0]})\n")

    success = False
    if not args.ctypes:
        success = test_via_pybind()

    if not success or args.ctypes:
        success = test_via_ctypes(args.dll)

    if success:
        print("=== All reachable mylib entry points verified successfully ===")
    else:
        print("=== Verification completed with some paths skipped (rebuild cpp-core if needed) ===")


if __name__ == "__main__":
    main()
