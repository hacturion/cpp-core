#pragma once

#include <cstddef>
#include <vector>

namespace mylib {
namespace tension_spline {

// Return codes (match mylib.h C API).
constexpr int kOk = 0;
constexpr int kBadArgs = -1;
constexpr int kInvalidData = -2;
constexpr int kSingular = -3;
constexpr int kOutOfRange = -5;

bool IsStrictlyIncreasing(const double* t, int n);
bool HasFiniteValues(const double* data, int n);

// Solve for second-derivative multipliers Z (size n).
int SolveZ(const double* t, const double* y, int n, double tau, std::vector<double>& z_out);

// Interval index i with t[i] <= x <= t[i+1]; returns -1 if x is outside [t[0], t[n]].
int FindInterval(const double* t, int n, double x);

// Evaluate the tension spline on interval i (0 <= i < n-1).
double EvaluateOnInterval(
    const double* t,
    const double* y,
    int n,
    const std::vector<double>& z,
    double tau,
    double x,
    int interval
);

// Evaluate at x using a precomputed Z vector.
double Evaluate(
    const double* t,
    const double* y,
    int n,
    const std::vector<double>& z,
    double tau,
    double x
);

// Build Z once, evaluate each query point.
int EvaluateBatch(
    const double* t,
    const double* y,
    int n,
    double tau,
    const double* query_x,
    int n_query,
    double* result_out
);

} // namespace tension_spline
} // namespace mylib