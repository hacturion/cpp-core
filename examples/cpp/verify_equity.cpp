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

int main() {
    const double tenors[] = {0.25, 1.0, 5.0, 10.0};
    const double rates[] = {0.02, 0.025, 0.03, 0.035};
    char zc_id[24] = {};

    check(mylib_zero_curve_create(tenors, rates, 4, zc_id, static_cast<int>(sizeof(zc_id))) == 0,
          "zero curve create");

    double call = 0.0;
    check(mylib_equity_option_npv(100.0, 100.0, 0.20, 1.0, zc_id, 1, 0.0, &call) == 0,
          "equity call return code");
    check(call > 0.0 && std::isfinite(call), "equity call positive");

    double put = 0.0;
    check(mylib_equity_option_npv(100.0, 100.0, 0.20, 1.0, zc_id, -1, 0.0, &put) == 0,
          "equity put return code");
    check(put > 0.0 && std::isfinite(put), "equity put positive");

    double cb = 0.0;
    check(mylib_convertible_bond_npv(
              100.0, 100.0, 0.02, 5.0, 2.0, 0.25, 0.01, zc_id, 0.0, &cb) == 0,
          "convertible bond return code");
    check(cb > 0.0 && std::isfinite(cb), "convertible bond positive");

    mylib_zero_curve_release(zc_id);
    std::printf("%s: %d failure(s)\n", failures == 0 ? "PASS" : "FAIL", failures);
    return failures == 0 ? 0 : 1;
}