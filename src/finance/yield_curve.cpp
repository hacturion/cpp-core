#include "mylib.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

#ifdef MYLIB_HAS_QUANTLIB
#include <ql/math/interpolations/linearinterpolation.hpp>
#include <ql/settings.hpp>
#include <ql/time/calendar.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <ql/termstructures/yield/zerocurve.hpp>
#include <ql/utilities/null.hpp>
#endif

#ifdef MYLIB_HAS_QUANTLIB

namespace {

using Curve = QuantLib::InterpolatedZeroCurve<QuantLib::Linear>;

std::optional<Curve> BuildCurve(
    const double* tenors,
    const double* zero_rates,
    int n_pillars
) {
    if (!tenors || !zero_rates || n_pillars < 2) return std::nullopt;

    std::vector<QuantLib::Time> times;
    std::vector<QuantLib::Rate> rates;
    times.reserve(static_cast<size_t>(n_pillars));
    rates.reserve(static_cast<size_t>(n_pillars));

    for (int i = 0; i < n_pillars; ++i) {
        if (tenors[i] <= 0.0) return std::nullopt;
        times.push_back(tenors[i]);
        rates.push_back(zero_rates[i]);
    }

    std::vector<size_t> order(times.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        return times[a] < times[b];
    });

    std::vector<QuantLib::Time> sorted_times;
    std::vector<QuantLib::Rate> sorted_rates;
    sorted_times.reserve(times.size());
    sorted_rates.reserve(rates.size());
    for (size_t idx : order) {
        if (!sorted_times.empty() && times[idx] == sorted_times.back()) {
            sorted_rates.back() = rates[idx];
            continue;
        }
        sorted_times.push_back(times[idx]);
        sorted_rates.push_back(rates[idx]);
    }

    if (sorted_times.size() < 2) return std::nullopt;

    const QuantLib::Date ref = QuantLib::Settings::instance().evaluationDate();
    std::vector<QuantLib::Date> dates;
    dates.reserve(sorted_times.size());
    for (QuantLib::Time t : sorted_times) {
        dates.push_back(ref + static_cast<QuantLib::Integer>(std::lround(t * 365.25)));
    }

    return Curve(
        dates,
        sorted_rates,
        QuantLib::ActualActual(QuantLib::ActualActual::ISDA),
        QuantLib::Linear(),
        QuantLib::Continuous,
        QuantLib::Annual
    );
}

} // namespace

extern "C" int mylib_zero_curve_discount(
    double t,
    const double* tenors,
    const double* zero_rates,
    int n_pillars,
    double* df_out
) {
    if (!df_out || t < 0.0) return -1;

    const auto curve = BuildCurve(tenors, zero_rates, n_pillars);
    if (!curve) return -1;

    *df_out = curve->discount(t, true);
    return 0;
}

extern "C" int mylib_zero_curve_discounts(
    const double* times,
    int n_times,
    const double* tenors,
    const double* zero_rates,
    int n_pillars,
    double* dfs_out
) {
    if (!times || !dfs_out || n_times <= 0) return -1;

    const auto curve = BuildCurve(tenors, zero_rates, n_pillars);
    if (!curve) return -1;

    for (int i = 0; i < n_times; ++i) {
        if (times[i] < 0.0) return -3;
        dfs_out[i] = curve->discount(times[i], true);
    }
    return 0;
}

extern "C" int mylib_zero_curve_rate(
    double t,
    const double* tenors,
    const double* zero_rates,
    int n_pillars,
    double* rate_out
) {
    if (!rate_out || t < 0.0) return -1;

    const auto curve = BuildCurve(tenors, zero_rates, n_pillars);
    if (!curve) return -1;

    *rate_out = curve->zeroRate(t, QuantLib::Continuous, QuantLib::Annual, true);
    return 0;
}

#else

extern "C" int mylib_zero_curve_discount(
    double /*t*/,
    const double* /*tenors*/,
    const double* /*zero_rates*/,
    int /*n_pillars*/,
    double* /*df_out*/
) {
    return -4;
}

extern "C" int mylib_zero_curve_discounts(
    const double* /*times*/,
    int /*n_times*/,
    const double* /*tenors*/,
    const double* /*zero_rates*/,
    int /*n_pillars*/,
    double* /*dfs_out*/
) {
    return -4;
}

extern "C" int mylib_zero_curve_rate(
    double /*t*/,
    const double* /*tenors*/,
    const double* /*zero_rates*/,
    int /*n_pillars*/,
    double* /*rate_out*/
) {
    return -4;
}

#endif