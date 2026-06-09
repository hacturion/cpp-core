# Using mylib from finsoft-sim (or other Python projects)

The C++ numerical core (mylib) is designed to be called from the bank/hedge fund simulator (finsoft-sim) or any other Python code.

## Quick Integration Steps

1. Build the library (see `BUILD_INSTRUCTIONS.md` or run `rebuild.ps1` from the correct VS prompt).

2. In a Python environment that can see the build output:

   ```python
   import sys
   sys.path.insert(0, r"C:\Users\James\projects\cpp-core\build\Release")  # or set PYTHONPATH

   import mylib
   import numpy as np

   # Example: better quasi-random numbers for a prop book simulation
   sobol = mylib.random.make_sobol(8)
   points = np.array([sobol.next() for _ in range(1000)])

   price = mylib.european_call_mc(S0=100, K=100, r=0.05, sigma=0.2, T=1.0,
                                  n_paths=50000, n_steps=1, use_sobol=True)
   ```

3. In finsoft-sim's hedge fund P&L calculation (`src/finsoft_sim/simulation/engine.py` `_compute_hedge_fund_pnl`), you can replace or augment the simplified `alpha * capacity` logic with real MC / Sobol paths driven by the market state factors.

## Recommended Pattern
- Keep heavy numerics in C++.
- Use pybind11 path from notebooks / finsoft engine.
- Use the Excel-DNA or xlwings path when you want the numbers directly in spreadsheets.

See also:
- `examples/python/verify_mylib.py`
- `examples/python/xlwings/udfs.py` (for Excel)
- `examples/cpp/monte_carlo_european.cpp` (fuller C++ example)
