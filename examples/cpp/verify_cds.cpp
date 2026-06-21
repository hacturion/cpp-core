// Quick verification for CDS curve cache and pricing.
#include "mylib.h"

#include <cmath>
#include <cstdio>
#include <cstring>

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

static void test_cached_curve_flow() {
    const double disc_tenors[] = {0.25, 1.0, 3.0, 5.0, 10.0};
    const double disc_rates[] = {0.02, 0.025, 0.03, 0.032, 0.035};
    const int n_disc = 5;

    const double cds_tenors[] = {1.0, 3.0, 5.0, 10.0};
    const double cds_spreads[] = {0.0100, 0.0120, 0.0140, 0.0160};
    const int n_cds = 4;
    const double recovery = 0.40;

    char disc_curve_id[24] = {};
    check(mylib_zero_curve_create(
              disc_tenors, disc_rates, n_disc,
              disc_curve_id, static_cast<int>(sizeof(disc_curve_id))) == 0,
          "zero curve create return code");
    check(std::strncmp(disc_curve_id, "ZC-", 3) == 0, "zero curve id prefix");

    char curve_id[24] = {};
    check(mylib_cds_curve_create(
              cds_tenors, cds_spreads, n_cds, recovery,
              disc_curve_id,
              curve_id, static_cast<int>(sizeof(curve_id))) == 0,
          "curve create return code");
    check(std::strncmp(curve_id, "CDS-", 4) == 0, "curve id prefix");

    int n = 0;
    double hazard_times[4] = {};
    double hazard_rates[4] = {};
    check(mylib_cds_curve_hazard(
              curve_id, hazard_times, hazard_rates, 4, &n) == 0,
          "curve hazard return code");
    check(n == n_cds, "curve hazard count");
    for (int i = 0; i < n_cds; ++i) {
        check(approx(hazard_times[i], cds_tenors[i]), "cached hazard pillar time");
        check(hazard_rates[i] > 0.0 && std::isfinite(hazard_rates[i]), "cached positive hazard");
    }

    const double query_times[] = {0.5, 2.5, 5.0, 8.0};
    double survival[4] = {};
    check(mylib_cds_curve_survival(curve_id, query_times, 4, survival) == 0,
          "cached survival return code");
    for (int i = 0; i < 4; ++i) {
        check(survival[i] > 0.0 && survival[i] <= 1.0, "cached survival in (0,1]");
    }

    double npv = 0.0;
    double fair_spread = 0.0;
    check(mylib_cds_curve_npv(curve_id, 5.0, 0.0140, 10000000.0, 1, &npv, &fair_spread) == 0,
          "cached npv return code");
    check(std::isfinite(npv), "cached npv finite");
    check(std::isfinite(fair_spread) && fair_spread > 0.0, "cached fair spread finite");

    double npv_fair = 0.0;
    check(mylib_cds_curve_npv(curve_id, 5.0, fair_spread, 10000000.0, 1, &npv_fair, nullptr) == 0,
          "cached npv at fair spread return code");
    check(std::fabs(npv_fair) < 5000.0, "cached npv near zero at fair spread");

    check(mylib_cds_curve_release(curve_id) == 0, "curve release return code");
    check(mylib_cds_curve_survival(curve_id, query_times, 1, survival) == -5,
          "unknown curve id rejected");
    check(mylib_zero_curve_release(disc_curve_id) == 0, "zero curve release return code");
}

static void test_stateless_bootstrap_still_works() {
    const double disc_tenors[] = {1.0, 5.0, 10.0};
    const double disc_rates[] = {0.03, 0.035, 0.04};
    const double cds_tenors[] = {1.0, 5.0};
    const double cds_spreads[] = {0.01, 0.015};
    double hazard_times[2] = {};
    double hazard_rates[2] = {};

    check(mylib_cds_bootstrap_curve(
              cds_tenors, cds_spreads, 2, 0.4,
              disc_tenors, disc_rates, 3,
              hazard_times, hazard_rates) == 0,
          "stateless bootstrap return code");
}

static void test_error_paths() {
    char curve_id[24] = {};
    const double cds_tenors[] = {1.0, 5.0};
    const double cds_spreads[] = {0.01, 0.015};

    check(mylib_cds_curve_create(
              nullptr, cds_spreads, 2, 0.4, "ZC-00000001",
              curve_id, static_cast<int>(sizeof(curve_id))) == -1,
          "null cds tenors rejected");
    check(mylib_cds_curve_create(
              cds_tenors, cds_spreads, 2, 0.4, "ZC-99999999",
              curve_id, static_cast<int>(sizeof(curve_id))) == -5,
          "missing zero curve id rejected");
    check(mylib_cds_curve_release("CDS-99999999") == -5, "missing curve release rejected");
}

int main() {
    std::printf("Verifying CDS curve cache and pricer...\n");
    test_cached_curve_flow();
    test_stateless_bootstrap_still_works();
    test_error_paths();
    std::printf("%s: %d failure(s)\n", failures == 0 ? "PASS" : "FAIL", failures);
    return failures == 0 ? 0 : 1;
}