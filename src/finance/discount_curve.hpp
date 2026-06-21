#pragma once

#ifdef MYLIB_HAS_QUANTLIB

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

#include <ql/math/interpolations/linearinterpolation.hpp>
#include <ql/settings.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <ql/termstructures/yield/zerocurve.hpp>

namespace mylib::finance {

using DiscountCurve = QuantLib::InterpolatedZeroCurve<QuantLib::Linear>;

inline void SetEvaluationDate() {
    QuantLib::Settings::instance().evaluationDate() = QuantLib::Date::todaysDate();
}

inline std::optional<DiscountCurve> BuildDiscountCurve(
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
        if (!std::isfinite(tenors[i]) || !std::isfinite(zero_rates[i]) || tenors[i] <= 0.0) {
            return std::nullopt;
        }
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
    std::vector<QuantLib::Rate> curve_rates = sorted_rates;
    dates.reserve(sorted_times.size() + 1);
    dates.push_back(ref);
    curve_rates.insert(curve_rates.begin(), sorted_rates.front());
    for (QuantLib::Time t : sorted_times) {
        dates.push_back(ref + static_cast<QuantLib::Integer>(std::lround(t * 365.25)));
    }

    return DiscountCurve(
        dates,
        curve_rates,
        QuantLib::ActualActual(QuantLib::ActualActual::ISDA),
        QuantLib::Linear(),
        QuantLib::Continuous,
        QuantLib::Annual
    );
}

// Returns a handle to a cached zero curve (ZC-...) or an empty handle if not found.
QuantLib::Handle<QuantLib::YieldTermStructure> LookupZeroCurve(const char* curve_id);

} // namespace mylib::finance

#endif