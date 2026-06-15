#pragma once

// mylib.h
// Public C ABI for the numerical core library.
//
// This is intentionally minimal and stable. It is used for:
// - Excel (via P/Invoke from C# + Excel-DNA)
// - Python (via ctypes, when you don't want pybind11)
// - Other languages
//
// For rich Python usage, prefer the pybind11 bindings in python/bindings.cpp.

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MYLIB_STATIC
  #define MYLIB_API
#elif defined(_WIN32)
  #ifdef MYLIB_EXPORTS
    #define MYLIB_API __declspec(dllexport)
  #else
    #define MYLIB_API __declspec(dllimport)
  #endif
#else
  #define MYLIB_API
#endif

// ============================================================================
// Library identity
// ============================================================================
MYLIB_API const char* mylib_version(void);

// ============================================================================
// Basic reductions (numerically careful where it matters)
// ============================================================================

// Sum of a vector using Kahan compensated summation for better accuracy.
MYLIB_API double mylib_sum(const double* data, int length);

// Weighted sum: sum(w[i] * x[i]). Useful in finance (e.g. discounted payoffs).
MYLIB_API double mylib_weighted_sum(
    const double* values,
    const double* weights,
    int length
);

// Trivial wrappers (kept for compatibility with Excel-DNA P/Invoke and ctypes examples)
MYLIB_API double mylib_add(double a, double b);
MYLIB_API double mylib_multiply(double a, double b);

// ============================================================================
// Basic array utilities (will grow into a proper math module)
// ============================================================================

MYLIB_API void mylib_add_arrays(
    const double* a,
    const double* b,
    double* result,
    int length
);

// Simple moving average (useful for diagnostics / smoothing)
MYLIB_API void mylib_moving_average(
    const double* input,
    double* output,
    int length,
    int window
);

// Basic descriptive statistics
// stats_out must have space for at least 4 doubles: [min, max, mean, std]
MYLIB_API int mylib_basic_stats(
    const double* data,
    int length,
    double* stats_out
);

// Matrix multiply: C = A * B (row-major storage).
// A is rows_a x cols_a, B is rows_b x cols_b; requires cols_a == rows_b.
// result must hold rows_a * cols_b doubles.
// Returns 0 on success, negative on error.
MYLIB_API int mylib_matrix_multiply(
    const double* a, int rows_a, int cols_a,
    const double* b, int rows_b, int cols_b,
    double* result
);

// ============================================================================
// Quasi-Monte Carlo (Boost Sobol when MYLIB_HAS_BOOST is defined)
// ============================================================================

// Generate n_points Sobol points in [0,1)^dimension (row-major: point i at out[i*dim + d]).
// skip advances the sequence before generation (useful for parallel streams).
// Returns 0 on success; negative on error (-4 if Boost was not linked).
MYLIB_API int mylib_sobol_points(
    int dimension,
    int n_points,
    unsigned long long skip,
    double* out
);

// ============================================================================
// Yield curves (QuantLib when MYLIB_HAS_QUANTLIB is defined)
// ============================================================================

// Pillars are tenors in years; zero_rates are continuously-compounded annual rates (decimal).
// Linear interpolation in zero rates (Actual/Actual ISDA day count).

// Discount factor DF(t) for a single maturity t (years).
MYLIB_API int mylib_zero_curve_discount(
    double t,
    const double* tenors,
    const double* zero_rates,
    int n_pillars,
    double* df_out
);

// Discount factors for multiple maturities.
MYLIB_API int mylib_zero_curve_discounts(
    const double* times,
    int n_times,
    const double* tenors,
    const double* zero_rates,
    int n_pillars,
    double* dfs_out
);

// Zero rate (continuous, annual) at maturity t (years).
MYLIB_API int mylib_zero_curve_rate(
    double t,
    const double* tenors,
    const double* zero_rates,
    int n_pillars,
    double* rate_out
);

// ============================================================================
// Stateful context (example pattern - will be replaced by proper classes later)
// ============================================================================
typedef struct MyLibHandle_* MyLibHandle;

MYLIB_API MyLibHandle mylib_create_context(int initial_capacity);
MYLIB_API void mylib_destroy_context(MyLibHandle handle);
MYLIB_API int mylib_context_add_value(MyLibHandle handle, double value);
MYLIB_API double mylib_context_get_mean(MyLibHandle handle);

#ifdef __cplusplus
}
#endif
