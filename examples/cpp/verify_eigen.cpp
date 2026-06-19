// Quick verification for mylib_symmetric_eigenvalues / mylib_symmetric_eigenvectors.
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
    // Symmetric [[4, 2], [2, 3]] has eigenvalues (7 - sqrt(17))/2 and (7 + sqrt(17))/2.
    const double a[] = {4.0, 2.0, 2.0, 3.0};
    const int n = 2;
    double vals[2] = {};
    double vecs[4] = {};

    check(mylib_symmetric_eigenvalues(a, n, vals) == 0, "2x2 eigenvalues return code");
    check(mylib_symmetric_eigenvectors(a, n, vecs) == 0, "2x2 eigenvectors return code");

    const double expected0 = 0.5 * (7.0 - std::sqrt(17.0));
    const double expected1 = 0.5 * (7.0 + std::sqrt(17.0));
    check(approx(vals[0], expected0), "2x2 smallest eigenvalue");
    check(approx(vals[1], expected1), "2x2 largest eigenvalue");
    check(vals[0] <= vals[1], "2x2 ascending order");

    for (int j = 0; j < n; ++j) {
        double norm2 = 0.0;
        double av0 = 0.0;
        double av1 = 0.0;
        for (int i = 0; i < n; ++i) {
            const double v_i = vecs[i * n + j];
            norm2 += v_i * v_i;
            av0 += a[0 * n + i] * v_i;
            av1 += a[1 * n + i] * v_i;
        }
        check(approx(norm2, 1.0, 1e-7), j == 0 ? "2x2 eigenvector 0 unit length" : "2x2 eigenvector 1 unit length");
        check(approx(av0, vals[j] * vecs[0 * n + j], 1e-7), j == 0 ? "2x2 eigenpair 0 row 0" : "2x2 eigenpair 1 row 0");
        check(approx(av1, vals[j] * vecs[1 * n + j], 1e-7), j == 0 ? "2x2 eigenpair 0 row 1" : "2x2 eigenpair 1 row 1");
    }

    double dot01 = vecs[0 * n + 0] * vecs[0 * n + 1] + vecs[1 * n + 0] * vecs[1 * n + 1];
    check(approx(dot01, 0.0, 1e-7), "2x2 eigenvectors orthogonal");
}

static void test_asymmetric_rejected() {
    const double a[] = {4.0, 3.0, 1.0, 3.0};
    double vals[2] = {};
    double vecs[4] = {};
    double max_asym = 0.0;
    double tolerance = 0.0;
    check(mylib_matrix_symmetry_metrics(a, 2, &max_asym, &tolerance) == -2, "asymmetric metrics rejected");
    check(max_asym == 2.0, "asymmetric max |A-A'|");
    check(mylib_symmetric_eigenvalues(a, 2, vals) == -2, "asymmetric eigenvalues rejected");
    check(mylib_symmetric_eigenvectors(a, 2, vecs) == -2, "asymmetric eigenvectors rejected");
}

static void test_identity() {
    const double a[] = {1.0, 0.0, 0.0, 1.0};
    double vals[2] = {};
    double vecs[4] = {};
    check(mylib_symmetric_eigenvalues(a, 2, vals) == 0, "identity eigenvalues");
    check(mylib_symmetric_eigenvectors(a, 2, vecs) == 0, "identity eigenvectors");
    check(approx(vals[0], 1.0) && approx(vals[1], 1.0), "identity eigenvalues are 1");

    for (int j = 0; j < 2; ++j) {
        double norm2 = vecs[0 * 2 + j] * vecs[0 * 2 + j] + vecs[1 * 2 + j] * vecs[1 * 2 + j];
        check(approx(norm2, 1.0, 1e-7), j == 0 ? "identity eigenvector 0 unit length" : "identity eigenvector 1 unit length");
    }
}

static void test_vals_vecs_consistency() {
    const double a[] = {4.0, 2.0, 2.0, 3.0};
    double vals_a[2] = {};
    double vals_b[2] = {};
    double vecs[4] = {};
    check(mylib_symmetric_eigenvalues(a, 2, vals_a) == 0, "consistency eigenvalues");
    check(mylib_symmetric_eigenvectors(a, 2, vecs) == 0, "consistency eigenvectors");
    check(mylib_symmetric_eigenvalues(a, 2, vals_b) == 0, "consistency eigenvalues repeat");
    check(approx(vals_a[0], vals_b[0]) && approx(vals_a[1], vals_b[1]), "repeated eigenvalue calls match");
}

static void test_error_paths() {
    const double a[] = {1.0, 0.0, 0.0, 1.0};
    double vals[2] = {};
    check(mylib_symmetric_eigenvalues(nullptr, 2, vals) == -1, "null matrix rejected");
    check(mylib_symmetric_eigenvalues(a, 2, nullptr) == -1, "null output rejected");
    check(mylib_symmetric_eigenvalues(a, 0, vals) == -1, "n=0 rejected");
}

int main() {
    std::printf("Verifying symmetric eigen functions...\n");
    test_known_2x2();
    test_asymmetric_rejected();
    test_identity();
    test_vals_vecs_consistency();
    test_error_paths();
    std::printf("%s: %d failure(s)\n", failures == 0 ? "PASS" : "FAIL", failures);
    return failures == 0 ? 0 : 1;
}