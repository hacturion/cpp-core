#include "mylib/tension_spline.hpp"

#include "mylib.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace mylib {
namespace tension_spline {

namespace {

constexpr double kPivotTol = 1e-12;
constexpr double kStableSinhThreshold = 20.0;

// sinh(a)/sinh(b) without overflow when a,b are large (e.g. high tau * interval width).
double SinhRatio(double a, double b) {
    if (b == 0.0) {
        return a == 0.0 ? 1.0 : 0.0;
    }
    if (std::max(std::abs(a), std::abs(b)) < kStableSinhThreshold) {
        return std::sinh(a) / std::sinh(b);
    }
    const double em2a = std::exp(-2.0 * a);
    const double em2b = std::exp(-2.0 * b);
    return std::exp(a - b) * (1.0 - em2a) / (1.0 - em2b);
}

// tau / sinh(tau*h) for the tridiagonal coefficients.
double TauOverSinh(double tau, double h) {
    const double s = tau * h;
    if (s < kStableSinhThreshold) {
        return tau / std::sinh(s);
    }
    const double em2s = std::exp(-2.0 * s);
    return 2.0 * tau * std::exp(-s) / (1.0 - em2s);
}

// coth(tau*h) for the tridiagonal coefficients.
double Coth(double s) {
    if (s < kStableSinhThreshold) {
        const double sh = std::sinh(s);
        if (sh == 0.0) return std::numeric_limits<double>::infinity();
        return std::cosh(s) / sh;
    }
    return 1.0 + 2.0 / (std::exp(2.0 * s) - 1.0);
}

bool SolveLinearSystem(std::vector<std::vector<double>>& mat, std::vector<double>& rhs) {
    const std::size_t m = mat.size();
    if (m == 0 || rhs.size() != m) return false;

    for (std::size_t i = 0; i < m; ++i) {
        std::size_t pivot = i;
        for (std::size_t j = i + 1; j < m; ++j) {
            if (std::abs(mat[j][i]) > std::abs(mat[pivot][i])) {
                pivot = j;
            }
        }

        if (pivot != i) {
            std::swap(mat[i], mat[pivot]);
            std::swap(rhs[i], rhs[pivot]);
        }

        if (std::abs(mat[i][i]) < kPivotTol) {
            return false;
        }

        for (std::size_t j = i + 1; j < m; ++j) {
            const double factor = mat[j][i] / mat[i][i];
            for (std::size_t k = i; k < m; ++k) {
                mat[j][k] -= factor * mat[i][k];
            }
            rhs[j] -= factor * rhs[i];
        }
    }

    std::vector<double> sol(m, 0.0);
    for (int i = static_cast<int>(m) - 1; i >= 0; --i) {
        double sum = 0.0;
        for (std::size_t j = static_cast<std::size_t>(i) + 1; j < m; ++j) {
            sum += mat[static_cast<std::size_t>(i)][j] * sol[j];
        }
        sol[static_cast<std::size_t>(i)] = (rhs[static_cast<std::size_t>(i)] - sum) / mat[static_cast<std::size_t>(i)][i];
    }

    rhs = std::move(sol);
    return true;
}

} // namespace

bool IsStrictlyIncreasing(const double* t, int n) {
    if (!t || n < 2) return false;
    for (int i = 1; i < n; ++i) {
        if (!std::isfinite(t[i]) || !(t[i] > t[i - 1])) return false;
    }
    return std::isfinite(t[0]);
}

bool HasFiniteValues(const double* data, int n) {
    if (!data || n <= 0) return false;
    for (int i = 0; i < n; ++i) {
        if (!std::isfinite(data[i])) return false;
    }
    return true;
}

int SolveZ(const double* t, const double* y, int n, double tau, std::vector<double>& z_out) {
    if (!t || !y || n < 2) return kBadArgs;
    if (!std::isfinite(tau) || tau <= 0.0) return kInvalidData;
    if (!IsStrictlyIncreasing(t, n) || !HasFiniteValues(y, n)) return kInvalidData;

    const std::size_t intervals = static_cast<std::size_t>(n) - 1;
    const std::size_t m = intervals + 1;

    std::vector<double> h(intervals);
    std::vector<double> a(intervals);
    std::vector<double> b(intervals);
    std::vector<double> g(intervals);

    for (std::size_t i = 0; i < intervals; ++i) {
        h[i] = t[i + 1] - t[i];
        g[i] = tau * tau * (y[i + 1] - y[i]) / h[i];

        const double inv_h = 1.0 / h[i];
        const double s = tau * h[i];
        a[i] = inv_h - TauOverSinh(tau, h[i]);
        b[i] = tau * Coth(s) - inv_h;

        if (!std::isfinite(g[i]) || !std::isfinite(a[i]) || !std::isfinite(b[i])) {
            return kInvalidData;
        }
    }

    std::vector<std::vector<double>> matrix(m, std::vector<double>(m, 0.0));
    for (std::size_t row = 1; row < intervals; ++row) {
        matrix[row][row - 1] = a[row - 1];
        matrix[row][row] = b[row - 1] + b[row];
        matrix[row][row + 1] = a[row];
    }
    matrix[0][0] = 1.0;
    matrix[intervals][intervals] = 1.0;

    std::vector<double> rhs(m, 0.0);
    for (std::size_t i = 1; i < intervals; ++i) {
        rhs[i] = g[i] - g[i - 1];
    }

    if (!SolveLinearSystem(matrix, rhs)) {
        return kSingular;
    }

    for (double z_i : rhs) {
        if (!std::isfinite(z_i)) return kInvalidData;
    }

    z_out = std::move(rhs);
    return kOk;
}

int FindInterval(const double* t, int n, double x) {
    if (!t || n < 2 || !std::isfinite(x)) return -1;
    if (x < t[0] || x > t[n - 1]) return -1;

    const auto begin = t;
    const auto end = t + n;
    const auto it = std::upper_bound(begin, end, x);
    int idx = static_cast<int>(it - begin) - 1;
    if (idx < 0) idx = 0;
    if (idx >= n - 1) idx = n - 2;
    return idx;
}

double EvaluateOnInterval(
    const double* t,
    const double* y,
    int n,
    const std::vector<double>& z,
    double tau,
    double x,
    int interval
) {
    const std::size_t i = static_cast<std::size_t>(interval);
    const std::size_t intervals = static_cast<std::size_t>(n) - 1;
    if (i >= intervals) return 0.0;

    const double h_i = t[i + 1] - t[i];
    const double tau_sq = tau * tau;
    const double alpha = tau * (t[i + 1] - x);
    const double beta = tau * (x - t[i]);
    const double s = tau * h_i;

    const double t1 = (z[i] * SinhRatio(alpha, s) + z[i + 1] * SinhRatio(beta, s)) / tau_sq;

    const double t2 = (y[i] - z[i] / tau_sq) * (t[i + 1] - x) / h_i;
    const double t3 = (y[i + 1] - z[i + 1] / tau_sq) * (x - t[i]) / h_i;

    return t1 + t2 + t3;
}

double Evaluate(
    const double* t,
    const double* y,
    int n,
    const std::vector<double>& z,
    double tau,
    double x
) {
    const int interval = FindInterval(t, n, x);
    if (interval < 0) return std::numeric_limits<double>::quiet_NaN();
    return EvaluateOnInterval(t, y, n, z, tau, x, interval);
}

int EvaluateBatch(
    const double* t,
    const double* y,
    int n,
    double tau,
    const double* query_x,
    int n_query,
    double* result_out
) {
    if (!t || !y || !query_x || !result_out || n < 2 || n_query <= 0) return kBadArgs;

    std::vector<double> z;
    const int build_rc = SolveZ(t, y, n, tau, z);
    if (build_rc != kOk) return build_rc;

    for (int i = 0; i < n_query; ++i) {
        if (!std::isfinite(query_x[i])) return kInvalidData;

        const int interval = FindInterval(t, n, query_x[i]);
        if (interval < 0) return kOutOfRange;

        result_out[i] = EvaluateOnInterval(t, y, n, z, tau, query_x[i], interval);
        if (!std::isfinite(result_out[i])) return kInvalidData;
    }

    return kOk;
}

} // namespace tension_spline
} // namespace mylib

extern "C" int mylib_tension_spline(
    const double* t,
    const double* y,
    int n,
    double tau,
    const double* query_x,
    int n_query,
    double* result_out
) {
    return mylib::tension_spline::EvaluateBatch(t, y, n, tau, query_x, n_query, result_out);
}