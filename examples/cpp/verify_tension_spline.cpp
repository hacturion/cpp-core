// Quick verification for mylib_tension_spline (user example data).
#include "mylib.h"

#include <cmath>
#include <cstdio>

static int failures = 0;

static void check(bool ok, const char* msg) {
    if (!ok) {
        std::printf("FAIL: %s\n", msg);
        ++failures;
    } else {
        std::printf("ok: %s\n", msg);
    }
}

static bool approx(double a, double b, double tol = 1e-6) {
    return std::fabs(a - b) <= tol;
}

static void test_user_example() {
    const double t[] = {-1.5, 1, 2, 20, 32, 40, 53, 60, 65, 70, 72, 75};
    const double y[] = {0, 1, 4, 8, 15, 15, 15, 9, 8, 7, 2, 1};
    const int n = 12;
    const double tau = 10.0;

    double out[12] = {};
    check(mylib_tension_spline(t, y, n, tau, t, n, out) == 0, "user example return code");

    for (int i = 0; i < n; ++i) {
        check(approx(out[i], y[i], 1e-5), i == 0 ? "exact at knot 0" :
            i == n - 1 ? "exact at last knot" : "exact at interior knot");
    }

    const double mids[] = {-0.25, 1.5, 11.0, 26.0, 36.0, 46.5, 56.5, 62.5, 67.5, 71.0, 73.5};
    double mid_out[11] = {};
    check(mylib_tension_spline(t, y, n, tau, mids, 11, mid_out) == 0, "mid-interval return code");
    for (int i = 0; i < 11; ++i) {
        check(std::isfinite(mid_out[i]), "mid-interval finite");
    }
}

static void test_high_tau() {
    const double t[] = {-1.5, 1, 2, 20, 32, 40, 53, 60, 65, 70, 72, 75};
    const double y[] = {0, 1, 4, 8, 15, 15, 15, 9, 8, 7, 2, 1};
    const int n = 12;
    const double taus[] = {105.0, 200.0, 1000.0};
    double out[12] = {};

    for (double tau : taus) {
        char label[64] = {};
        std::snprintf(label, sizeof(label), "high tau %.0f return code", tau);
        check(mylib_tension_spline(t, y, n, tau, t, n, out) == 0, label);
        for (int i = 0; i < n; ++i) {
            check(std::isfinite(out[i]), "high tau output finite");
            check(approx(out[i], y[i], 1e-4), "high tau exact at knot");
        }
    }
}

static void test_error_paths() {
    const double t[] = {0.0, 1.0, 2.0};
    const double y[] = {0.0, 1.0, 2.0};
    const double query[] = {0.5};
    double out[1] = {};

    check(mylib_tension_spline(nullptr, y, 3, 1.0, query, 1, out) == -1, "null t rejected");
    check(mylib_tension_spline(t, y, 1, 1.0, query, 1, out) == -1, "too few knots rejected");
    check(mylib_tension_spline(t, y, 3, 0.0, query, 1, out) == -2, "zero tau rejected");

    const double bad_t[] = {0.0, 0.0, 1.0};
    check(mylib_tension_spline(bad_t, y, 3, 1.0, query, 1, out) == -2, "non-increasing t rejected");

    const double outside[] = {3.0};
    check(mylib_tension_spline(t, y, 3, 1.0, outside, 1, out) == -5, "outside query rejected");
}

int main() {
    std::printf("Verifying tension spline...\n");
    test_user_example();
    test_high_tau();
    test_error_paths();
    std::printf("%s: %d failure(s)\n", failures == 0 ? "PASS" : "FAIL", failures);
    return failures == 0 ? 0 : 1;
}