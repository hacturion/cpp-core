// Quick verification for mylib_cholesky_decomposition.
#include "mylib.h"

#include <cmath>
#include <cstdio>
#include <vector>

static int failures = 0;

static void check(bool ok, const char* msg) {
    if (!ok) {
        std::printf("FAIL: %s\n", msg);
        ++failures;
    } else {
        std::printf("ok: %s\n", msg);
    }
}

static bool approx(double a, double b, double tol = 1e-8) {
    return std::fabs(a - b) <= tol;
}

static void test_known_2x2() {
    // Correlation matrix [[1, 0.5], [0.5, 1]] = L L' with L = [[1, 0], [0.5, sqrt(0.75)]].
    const double a[] = {1.0, 0.5, 0.5, 1.0};
    const int n = 2;
    double l[4] = {};

    check(mylib_cholesky_decomposition(a, n, l) == 0, "2x2 Cholesky return code");
    check(approx(l[0], 1.0), "2x2 L(0,0)");
    check(approx(l[1], 0.0), "2x2 L(0,1)");
    check(approx(l[2], 0.5), "2x2 L(1,0)");
    check(approx(l[3], std::sqrt(0.75)), "2x2 L(1,1)");

    double reconstructed[4] = {};
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double sum = 0.0;
            for (int k = 0; k < n; ++k) {
                sum += l[i * n + k] * l[j * n + k];
            }
            reconstructed[i * n + j] = sum;
        }
    }
    for (int i = 0; i < n * n; ++i) {
        check(approx(reconstructed[i], a[i]), i == 0 ? "2x2 L L' matches A (0,0)" :
            i == 1 ? "2x2 L L' matches A (0,1)" :
            i == 2 ? "2x2 L L' matches A (1,0)" : "2x2 L L' matches A (1,1)");
    }
}

static void test_not_correlation_matrix() {
    const double spd_not_corr[] = {4.0, 2.0, 2.0, 3.0};
    const double invalid_rho_high[] = {1.0, 1.2, 1.2, 1.0};
    const double invalid_rho_low[] = {1.0, -1.2, -1.2, 1.0};
    double l[4] = {};
    double max_diag = 0.0;
    double min_offdiag = 0.0;
    double max_offdiag = 0.0;
    double tol = 0.0;

    check(mylib_cholesky_decomposition(spd_not_corr, 2, l) == -5, "SPD non-correlation rejected");
    check(mylib_matrix_correlation_metrics(
              spd_not_corr, 2, &max_diag, &min_offdiag, &max_offdiag, &tol) == -5,
        "SPD non-correlation metrics rejected");
    check(max_diag > 0.0, "SPD non-correlation diagonal deviation");

    check(mylib_cholesky_decomposition(invalid_rho_high, 2, l) == -5, "rho > 1 rejected");
    check(mylib_matrix_correlation_metrics(
              invalid_rho_high, 2, &max_diag, &min_offdiag, &max_offdiag, &tol) == -5,
        "rho > 1 metrics rejected");
    check(max_offdiag > 1.0, "rho > 1 off-diagonal exceeds 1");

    check(mylib_cholesky_decomposition(invalid_rho_low, 2, l) == -5, "rho < -1 rejected");
    check(mylib_matrix_correlation_metrics(
              invalid_rho_low, 2, &max_diag, &min_offdiag, &max_offdiag, &tol) == -5,
        "rho < -1 metrics rejected");
    check(min_offdiag < -1.0, "rho < -1 off-diagonal below -1");
}

static void test_asymmetric_rejected() {
    const double a[] = {4.0, 3.0, 1.0, 3.0};
    double l[4] = {};
    check(mylib_cholesky_decomposition(a, 2, l) == -2, "asymmetric Cholesky rejected");
}

static void test_not_positive_definite() {
    // Valid correlation form but singular (rank 1).
    const double a[] = {1.0, 1.0, 1.0, 1.0};
    double l[4] = {};
    check(mylib_cholesky_decomposition(a, 2, l) == -3, "singular correlation rejected");
}

static void test_identity() {
    const double a[] = {1.0, 0.0, 0.0, 1.0};
    double l[4] = {};
    check(mylib_cholesky_decomposition(a, 2, l) == 0, "identity Cholesky");
    check(approx(l[0], 1.0) && approx(l[3], 1.0), "identity diagonal");
    check(approx(l[1], 0.0) && approx(l[2], 0.0), "identity off-diagonal zero");
}

static void test_error_paths() {
    const double a[] = {1.0, 0.0, 0.0, 1.0};
    double l[4] = {};
    double max_diag = 0.0;
    double min_offdiag = 0.0;
    double max_offdiag = 0.0;
    double tol = 0.0;
    check(mylib_cholesky_decomposition(nullptr, 2, l) == -1, "null matrix rejected");
    check(mylib_cholesky_decomposition(a, 2, nullptr) == -1, "null output rejected");
    check(mylib_cholesky_decomposition(a, 0, l) == -1, "n=0 rejected");
    check(mylib_matrix_correlation_metrics(
              a, 2, &max_diag, &min_offdiag, &max_offdiag, &tol) == 0,
        "identity passes correlation metrics");
}

int main() {
    std::printf("Verifying Cholesky decomposition...\n");
    test_known_2x2();
    test_not_correlation_matrix();
    test_asymmetric_rejected();
    test_not_positive_definite();
    test_identity();
    test_error_paths();
    std::printf("%s: %d failure(s)\n", failures == 0 ? "PASS" : "FAIL", failures);
    return failures == 0 ? 0 : 1;
}