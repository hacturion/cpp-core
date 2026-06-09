// mylib.cpp
// Implementation of the C++ core numerical library.
//
// Design goals:
// - Numerically careful implementations where it matters for finance
// - Stable C ABI for Excel and ctypes consumers
// - Clean modern C++ internals that we can evolve

#include "mylib.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#ifdef MYLIB_HAS_EIGEN
#include <Eigen/Dense>
#endif

#ifdef _WIN32
#define MYLIB_EXPORTS
#endif

// ============================================================================
// Library version
// ============================================================================
extern "C" const char* mylib_version(void) {
    return "mylib 0.2.0 (numerical methods core)";
}

// ============================================================================
// Numerically stable summation (Kahan compensated summation)
// ============================================================================
// This is significantly more accurate than naive summation when adding
// many values of very different magnitudes — common in Monte Carlo and
// discounted cash flow work.
extern "C" double mylib_sum(const double* data, int length) {
    if (!data || length <= 0) return 0.0;

    double sum = 0.0;
    double c = 0.0; // running compensation for lost low-order bits

    for (int i = 0; i < length; ++i) {
        double y = data[i] - c;
        double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }
    return sum;
}

// Weighted sum with the same compensated approach
extern "C" double mylib_weighted_sum(
    const double* values,
    const double* weights,
    int length
) {
    if (!values || !weights || length <= 0) return 0.0;

    double sum = 0.0;
    double c = 0.0;

    for (int i = 0; i < length; ++i) {
        double y = values[i] * weights[i] - c;
        double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }
    return sum;
}

// ============================================================================
// Basic array operations (to be expanded)
// ============================================================================
extern "C" void mylib_add_arrays(
    const double* a,
    const double* b,
    double* result,
    int length
) {
    if (!a || !b || !result || length <= 0) return;

    for (int i = 0; i < length; ++i) {
        result[i] = a[i] + b[i];
    }
}

extern "C" void mylib_moving_average(
    const double* input,
    double* output,
    int length,
    int window
) {
    if (!input || !output || length <= 0 || window <= 0) return;

    for (int i = 0; i < length; ++i) {
        double sum = 0.0;
        int count = 0;
        int start = std::max(0, i - window + 1);
        for (int j = start; j <= i; ++j) {
            sum += input[j];
            ++count;
        }
        output[i] = sum / count;
    }
}

extern "C" int mylib_basic_stats(
    const double* data,
    int length,
    double* stats_out
) {
    if (!data || length <= 0 || !stats_out) return -1;

    double min_val = data[0];
    double max_val = data[0];
    double sum = 0.0;

    for (int i = 0; i < length; ++i) {
        double v = data[i];
        if (v < min_val) min_val = v;
        if (v > max_val) max_val = v;
        sum += v;
    }

    double mean = sum / length;
    double variance = 0.0;

    for (int i = 0; i < length; ++i) {
        double diff = data[i] - mean;
        variance += diff * diff;
    }
    variance /= length;

    stats_out[0] = min_val;
    stats_out[1] = max_val;
    stats_out[2] = mean;
    stats_out[3] = std::sqrt(variance);

    return 0;
}

extern "C" int mylib_matrix_multiply(
    const double* a, int rows_a, int cols_a,
    const double* b, int rows_b, int cols_b,
    double* result
) {
    if (!a || !b || !result) return -1;
    if (rows_a <= 0 || cols_a <= 0 || rows_b <= 0 || cols_b <= 0) return -2;
    if (cols_a != rows_b) return -3;

    const int out_rows = rows_a;
    const int out_cols = cols_b;

#ifdef MYLIB_HAS_EIGEN
    using RowMat = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
    Eigen::Map<const RowMat> matA(a, rows_a, cols_a);
    Eigen::Map<const RowMat> matB(b, rows_b, cols_b);
    Eigen::Map<RowMat> matC(result, out_rows, out_cols);
    matC.noalias() = matA * matB;
#else
    for (int i = 0; i < out_rows; ++i) {
        for (int j = 0; j < out_cols; ++j) {
            double sum = 0.0;
            for (int k = 0; k < cols_a; ++k) {
                sum += a[i * cols_a + k] * b[k * out_cols + j];
            }
            result[i * out_cols + j] = sum;
        }
    }
#endif

    return 0;
}

// Opaque handle implementation (simple growing buffer + running stats)
struct MyLibHandle_ {
    std::vector<double> values;
};

extern "C" MyLibHandle mylib_create_context(int initial_capacity) {
    auto* ctx = new MyLibHandle_();
    if (initial_capacity > 0) {
        ctx->values.reserve(initial_capacity);
    }
    return ctx;
}

extern "C" void mylib_destroy_context(MyLibHandle handle) {
    delete handle;
}

extern "C" int mylib_context_add_value(MyLibHandle handle, double value) {
    if (!handle) return -1;
    handle->values.push_back(value);
    return 0;
}

extern "C" double mylib_context_get_mean(MyLibHandle handle) {
    if (!handle || handle->values.empty()) return 0.0;
    double sum = std::accumulate(handle->values.begin(), handle->values.end(), 0.0);
    return sum / static_cast<double>(handle->values.size());
}

// ============================================================================
// Trivial arithmetic (for API compatibility / simple NATIVE.* UDFs)
// ============================================================================
extern "C" double mylib_add(double a, double b) {
    return a + b;
}

extern "C" double mylib_multiply(double a, double b) {
    return a * b;
}
