# xlwings UDFs for mylib (Canonical Example)

This directory contains the **recommended / canonical** xlwings bridge for the C++ mylib numerical library.

## Why this location?
- Lives next to the C++ source and other examples.
- Single place to maintain the UDF wrappers.
- Other projects (NumericalTools, mylib_xlwings) can reference or copy `udfs.py`.

## Quick Start
1. Build the Python module:
   ```powershell
   cd cpp-core
   cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DMYLIB_BUILD_PYTHON=ON -DMYLIB_USE_EIGEN=ON
   cmake --build build --config Release
   ```

2. Make the .pyd importable (easiest for dev):
   ```powershell
   $env:PYTHONPATH = "build\Release"
   ```

3. Install xlwings if needed: `pip install xlwings`

4. In Excel:
   - Open your workbook (e.g. one of the .xlsm in sibling projects).
   - xlwings ribbon → Import Functions → select this `udfs.py`.
   - Use the functions: `=my_sum(A1:A1000)`, `=my_sobol_points(8, 20)`, etc.

## Functions Provided
- `my_sum` — Kahan compensated sum
- `my_weighted_sum` — for pricing / MC
- `my_add` / `my_multiply` — scalar (parity with NATIVE.*)
- `my_sobol_points(dimension, n_points)` — quasi-random (spills)
- `my_interval_example(lower, upper)`
- `debug_mylib()`

See `udfs.py` for full docstrings and how to add more.

## After Changing C++ Code
Rebuild cpp-core, copy the new `mylib.pyd` / `mylib.dll` as needed, then re-import the functions in Excel.

## Related
- Main docs: `../../BUILD_INSTRUCTIONS.md`
- Full roadmap: `../../ROADMAP.md`
- Native Excel-DNA path: `../../../excel-udf/`
- Other (older) xlwings setups: `../../../NumericalTools/`, `../../../mylib_xlwings/`
