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

// Matrix inverse for square n×n row-major A. Output is row-major A^{-1}.
// Returns 0 on success; -3 if singular/non-invertible; -4 if Eigen was not linked.
MYLIB_API int mylib_matrix_inverse(
    const double* matrix,
    int n,
    double* result
);

// Symmetry check for n×n row-major matrix.
// Writes max_{i<j} |A_ij - A_ji| and the tolerance used (1e-10 * max(1, max|A|)).
// Returns 0 if symmetric within tolerance, -2 if not, -4 if Eigen was not linked.
MYLIB_API int mylib_matrix_symmetry_metrics(
    const double* matrix,
    int n,
    double* max_asymmetry_out,
    double* tolerance_out
);

// Symmetric eigendecomposition (Eigen SelfAdjointEigenSolver when MYLIB_HAS_EIGEN).
// Input must be n×n row-major and symmetric (A = A'); non-symmetric matrices are rejected.
// Eigenvalues are real and returned in ascending order (n×1).
// Eigenvectors are orthonormal columns V; output is row-major n×n with V(:,j) in column j.
// Returns 0 on success; -2 if not symmetric; -3 if solver failed; -4 if Eigen was not linked.
MYLIB_API int mylib_symmetric_eigenvalues(
    const double* matrix,
    int n,
    double* eigenvalues_out
);

MYLIB_API int mylib_symmetric_eigenvectors(
    const double* matrix,
    int n,
    double* eigenvectors_out
);

// Correlation matrix check for n×n row-major matrix.
// Requires unit diagonal and off-diagonal entries in [-1, 1] (within tolerance).
// Writes max |diag_i - 1|, min/max off-diagonal A_ij (i != j), and tolerance (1e-10 * max(1, max|A|)).
// Returns 0 if valid; -5 if not a correlation matrix; -4 if Eigen was not linked.
MYLIB_API int mylib_matrix_correlation_metrics(
    const double* matrix,
    int n,
    double* max_diag_deviation_out,
    double* min_offdiag_out,
    double* max_offdiag_out,
    double* tolerance_out
);

// Cholesky decomposition A = L L' for n×n row-major correlation matrix A.
// Output is lower triangular L (row-major); upper triangle is zero.
// Returns 0 on success; -2 if not symmetric; -5 if not a correlation matrix;
// -3 if not positive definite; -4 if Eigen was not linked.
MYLIB_API int mylib_cholesky_decomposition(
    const double* matrix,
    int n,
    double* cholesky_out
);

// ============================================================================
// Interpolation
// ============================================================================

// Hyperbolic tension spline on strictly increasing knots (t[i], y[i]).
// tau > 0 controls tension (larger = straighter segments between knots).
// Evaluates at each query_x[j]; results written to result_out (length n_query).
// Requires n >= 2; query points must lie in [t[0], t[n-1]].
// Returns 0 on success; -1 bad args; -2 invalid knots/values/tau; -3 singular system; -5 query out of range.
MYLIB_API int mylib_tension_spline(
    const double* t,
    const double* y,
    int n,
    double tau,
    const double* query_x,
    int n_query,
    double* result_out
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
