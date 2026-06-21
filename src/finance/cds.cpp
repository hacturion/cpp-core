#include "mylib.h"

#include "discount_curve.hpp"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>

#ifdef MYLIB_HAS_QUANTLIB
#include <ql/default.hpp>
#include <ql/instruments/creditdefaultswap.hpp>
#include <ql/math/interpolations/linearinterpolation.hpp>
#include <ql/pricingengines/credit/midpointcdsengine.hpp>
#include <ql/settings.hpp>
#include <ql/termstructures/credit/defaultprobabilityhelpers.hpp>
#include <ql/termstructures/credit/piecewisedefaultcurve.hpp>
#include <ql/termstructures/credit/probabilitytraits.hpp>
#include <ql/time/calendars/weekendsonly.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/quotes/simplequote.hpp>
#endif

#ifdef MYLIB_HAS_QUANTLIB

namespace {

using HazardCurve = QuantLib::PiecewiseDefaultCurve<
    QuantLib::HazardRate,
    QuantLib::Linear>;

constexpr int kUnknownCurve = -5;
constexpr size_t kMaxCachedCurves = 1024;

struct CdsCurveEntry {
    QuantLib::ext::shared_ptr<HazardCurve> hazard;
    std::string disc_curve_id;
    double recovery_rate = 0.0;
    std::vector<double> cds_tenors;
};

std::mutex g_registry_mutex;
std::unordered_map<std::string, CdsCurveEntry> g_registry;
std::atomic<std::uint64_t> g_next_curve_id{1};

QuantLib::Period TenorFromYears(double years) {
    const int months = static_cast<int>(std::lround(years * 12.0));
    if (months <= 0) {
        return QuantLib::Period(1, QuantLib::Months);
    }
    return QuantLib::Period(months, QuantLib::Months);
}

bool ValidMarketPillars(const double* tenors, const double* values, int n) {
    if (!tenors || !values || n < 2) return false;
    for (int i = 0; i < n; ++i) {
        if (!std::isfinite(tenors[i]) || !std::isfinite(values[i]) || tenors[i] <= 0.0) {
            return false;
        }
        if (i > 0 && !(tenors[i] > tenors[i - 1])) return false;
    }
    return true;
}

QuantLib::Handle<QuantLib::YieldTermStructure> MakeDiscountHandle(
    const double* disc_tenors,
    const double* disc_rates,
    int n_disc
) {
    mylib::finance::SetEvaluationDate();
    const auto curve = mylib::finance::BuildDiscountCurve(disc_tenors, disc_rates, n_disc);
    if (!curve) {
        return {};
    }

    auto shared = QuantLib::ext::make_shared<mylib::finance::DiscountCurve>(*curve);
    shared->enableExtrapolation();
    return QuantLib::Handle<QuantLib::YieldTermStructure>(shared);
}

QuantLib::ext::shared_ptr<HazardCurve> BuildHazardCurve(
    const double* cds_tenors,
    const double* cds_spreads,
    int n_cds,
    double recovery_rate,
    const QuantLib::Handle<QuantLib::YieldTermStructure>& discount_curve
) {
    if (!ValidMarketPillars(cds_tenors, cds_spreads, n_cds)) return {};
    if (!std::isfinite(recovery_rate) || recovery_rate < 0.0 || recovery_rate >= 1.0) return {};
    if (discount_curve.empty()) return {};

    mylib::finance::SetEvaluationDate();

    const QuantLib::Calendar calendar = QuantLib::WeekendsOnly();
    const QuantLib::Date settlementDate = QuantLib::Settings::instance().evaluationDate();

    std::vector<QuantLib::ext::shared_ptr<QuantLib::DefaultProbabilityHelper>> helpers;
    helpers.reserve(static_cast<size_t>(n_cds));

    for (int i = 0; i < n_cds; ++i) {
        const auto quote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(cds_spreads[i]);
        helpers.push_back(QuantLib::ext::make_shared<QuantLib::SpreadCdsHelper>(
            QuantLib::Handle<QuantLib::Quote>(quote),
            TenorFromYears(cds_tenors[i]),
            0,
            calendar,
            QuantLib::Quarterly,
            QuantLib::Following,
            QuantLib::DateGeneration::CDS,
            QuantLib::Actual360(),
            recovery_rate,
            discount_curve));
    }

    auto curve = QuantLib::ext::make_shared<HazardCurve>(
        settlementDate,
        helpers,
        QuantLib::Actual365Fixed());
    curve->enableExtrapolation();

    (void)curve->hazardRate(cds_tenors[static_cast<size_t>(n_cds - 1)], true);
    return curve;
}

bool WriteCurveId(char* id_out, int id_cap, std::uint64_t id_number) {
    if (!id_out || id_cap <= 0) return false;
    const int written = std::snprintf(id_out, static_cast<size_t>(id_cap), "CDS-%08llu",
                                      static_cast<unsigned long long>(id_number));
    return written > 0 && written < id_cap;
}

const CdsCurveEntry* FindCurve(const char* curve_id) {
    if (!curve_id || !curve_id[0]) return nullptr;
    const auto it = g_registry.find(curve_id);
    return it == g_registry.end() ? nullptr : &it->second;
}

int PriceCds(
    const CdsCurveEntry& entry,
    const QuantLib::Handle<QuantLib::YieldTermStructure>& discount,
    double maturity_years,
    double running_spread,
    double notional,
    int side,
    double* npv_out,
    double* fair_spread_out
) {
    if (!npv_out) return -1;
    if (!std::isfinite(maturity_years) || maturity_years <= 0.0 ||
        !std::isfinite(running_spread) || !std::isfinite(notional) || notional <= 0.0 ||
        (side != 1 && side != -1)) {
        return -2;
    }
    if (discount.empty()) return kUnknownCurve;

    mylib::finance::SetEvaluationDate();

    const QuantLib::Calendar calendar = QuantLib::WeekendsOnly();
    const QuantLib::Date start = QuantLib::Settings::instance().evaluationDate();
    const QuantLib::Date maturity = calendar.advance(start, TenorFromYears(maturity_years));
    const QuantLib::Schedule schedule(
        start,
        maturity,
        QuantLib::Period(QuantLib::Quarterly),
        calendar,
        QuantLib::Following,
        QuantLib::Unadjusted,
        QuantLib::DateGeneration::CDS,
        false);

    const auto protection_side = side >= 0
        ? QuantLib::Protection::Buyer
        : QuantLib::Protection::Seller;

    QuantLib::CreditDefaultSwap cds(
        protection_side,
        notional,
        running_spread,
        schedule,
        QuantLib::Following,
        QuantLib::Actual360(),
        true,
        true,
        start);

    cds.setPricingEngine(QuantLib::ext::make_shared<QuantLib::MidPointCdsEngine>(
        QuantLib::Handle<QuantLib::DefaultProbabilityTermStructure>(entry.hazard),
        entry.recovery_rate,
        discount));

    const double npv = cds.NPV();
    if (!std::isfinite(npv)) return -3;

    *npv_out = npv;
    if (fair_spread_out) {
        *fair_spread_out = cds.fairSpread();
        if (!std::isfinite(*fair_spread_out)) return -3;
    }
    return 0;
}

} // namespace

extern "C" int mylib_cds_curve_create(
    const double* cds_tenors,
    const double* cds_spreads,
    int n_cds,
    double recovery_rate,
    const char* disc_curve_id,
    char* id_out,
    int id_cap
) {
    if (!cds_tenors || !cds_spreads || !disc_curve_id || !disc_curve_id[0] || !id_out || id_cap < 12 ||
        n_cds < 2) {
        return -1;
    }

    try {
        mylib::finance::SetEvaluationDate();

        const auto discount = mylib::finance::LookupZeroCurve(disc_curve_id);
        if (discount.empty()) return kUnknownCurve;

        const auto hazard = BuildHazardCurve(
            cds_tenors, cds_spreads, n_cds, recovery_rate, discount);
        if (!hazard) return -2;

        std::lock_guard<std::mutex> lock(g_registry_mutex);
        if (g_registry.size() >= kMaxCachedCurves) return -3;

        const std::uint64_t id_number = g_next_curve_id.fetch_add(1);
        if (!WriteCurveId(id_out, id_cap, id_number)) return -1;

        CdsCurveEntry entry;
        entry.hazard = hazard;
        entry.disc_curve_id = disc_curve_id;
        entry.recovery_rate = recovery_rate;
        entry.cds_tenors.assign(cds_tenors, cds_tenors + n_cds);
        g_registry.emplace(id_out, std::move(entry));
        return 0;
    } catch (...) {
        return -3;
    }
}

extern "C" int mylib_cds_curve_hazard(
    const char* curve_id,
    double* hazard_times_out,
    double* hazard_rates_out,
    int max_n,
    int* n_out
) {
    if (!curve_id || !hazard_times_out || !hazard_rates_out || !n_out || max_n <= 0) return -1;

    std::lock_guard<std::mutex> lock(g_registry_mutex);
    const CdsCurveEntry* entry = FindCurve(curve_id);
    if (!entry) return kUnknownCurve;

    const int n = static_cast<int>(entry->cds_tenors.size());
    if (n > max_n) return -1;
    *n_out = n;

    try {
        for (int i = 0; i < n; ++i) {
            hazard_times_out[i] = entry->cds_tenors[static_cast<size_t>(i)];
            hazard_rates_out[i] = entry->hazard->hazardRate(hazard_times_out[i], true);
            if (!std::isfinite(hazard_rates_out[i])) return -3;
        }
        return 0;
    } catch (...) {
        return -3;
    }
}

extern "C" int mylib_cds_curve_survival(
    const char* curve_id,
    const double* query_times,
    int n_query,
    double* survival_out
) {
    if (!curve_id || !query_times || !survival_out || n_query <= 0) return -1;

    std::lock_guard<std::mutex> lock(g_registry_mutex);
    const CdsCurveEntry* entry = FindCurve(curve_id);
    if (!entry) return kUnknownCurve;

    try {
        for (int i = 0; i < n_query; ++i) {
            if (!std::isfinite(query_times[i]) || query_times[i] < 0.0) return -2;
            survival_out[i] = entry->hazard->survivalProbability(query_times[i], true);
            if (!std::isfinite(survival_out[i])) return -3;
        }
        return 0;
    } catch (...) {
        return -3;
    }
}

extern "C" int mylib_cds_curve_npv(
    const char* curve_id,
    double maturity_years,
    double running_spread,
    double notional,
    int side,
    double* npv_out,
    double* fair_spread_out
) {
    if (!curve_id || !npv_out) return -1;

    std::lock_guard<std::mutex> lock(g_registry_mutex);
    const CdsCurveEntry* entry = FindCurve(curve_id);
    if (!entry) return kUnknownCurve;

    try {
        const auto discount = mylib::finance::LookupZeroCurve(entry->disc_curve_id.c_str());
        return PriceCds(
            *entry, discount, maturity_years, running_spread, notional, side, npv_out, fair_spread_out);
    } catch (...) {
        return -3;
    }
}

extern "C" int mylib_cds_curve_release(const char* curve_id) {
    if (!curve_id || !curve_id[0]) return -1;

    std::lock_guard<std::mutex> lock(g_registry_mutex);
    return g_registry.erase(curve_id) > 0 ? 0 : kUnknownCurve;
}

extern "C" void mylib_cds_curve_release_all(void) {
    std::lock_guard<std::mutex> lock(g_registry_mutex);
    g_registry.clear();
}

extern "C" int mylib_cds_bootstrap_curve(
    const double* cds_tenors,
    const double* cds_spreads,
    int n_cds,
    double recovery_rate,
    const double* disc_tenors,
    const double* disc_rates,
    int n_disc,
    double* hazard_times_out,
    double* hazard_rates_out
) {
    if (!cds_tenors || !cds_spreads || !disc_tenors || !disc_rates ||
        !hazard_times_out || !hazard_rates_out || n_cds < 2 || n_disc < 2) {
        return -1;
    }

    try {
        const auto discount = MakeDiscountHandle(disc_tenors, disc_rates, n_disc);
        const auto curve = BuildHazardCurve(
            cds_tenors, cds_spreads, n_cds, recovery_rate, discount);
        if (!curve) return -2;

        for (int i = 0; i < n_cds; ++i) {
            hazard_times_out[i] = cds_tenors[i];
            hazard_rates_out[i] = curve->hazardRate(cds_tenors[i], true);
            if (!std::isfinite(hazard_rates_out[i])) return -3;
        }
        return 0;
    } catch (...) {
        return -3;
    }
}

extern "C" int mylib_cds_survival_probs(
    const double* cds_tenors,
    const double* cds_spreads,
    int n_cds,
    double recovery_rate,
    const double* disc_tenors,
    const double* disc_rates,
    int n_disc,
    const double* query_times,
    int n_query,
    double* survival_out
) {
    if (!cds_tenors || !cds_spreads || !disc_tenors || !disc_rates ||
        !query_times || !survival_out || n_cds < 2 || n_disc < 2 || n_query <= 0) {
        return -1;
    }

    try {
        const auto discount = MakeDiscountHandle(disc_tenors, disc_rates, n_disc);
        const auto curve = BuildHazardCurve(
            cds_tenors, cds_spreads, n_cds, recovery_rate, discount);
        if (!curve) return -2;

        for (int i = 0; i < n_query; ++i) {
            if (!std::isfinite(query_times[i]) || query_times[i] < 0.0) return -2;
            survival_out[i] = curve->survivalProbability(query_times[i], true);
            if (!std::isfinite(survival_out[i])) return -3;
        }
        return 0;
    } catch (...) {
        return -3;
    }
}

extern "C" int mylib_cds_npv(
    double maturity_years,
    double running_spread,
    double notional,
    int side,
    double recovery_rate,
    const double* cds_tenors,
    const double* cds_spreads,
    int n_cds,
    const double* disc_tenors,
    const double* disc_rates,
    int n_disc,
    double* npv_out,
    double* fair_spread_out
) {
    if (!cds_tenors || !cds_spreads || !disc_tenors || !disc_rates || !npv_out ||
        n_cds < 2 || n_disc < 2) {
        return -1;
    }
    if (!std::isfinite(recovery_rate) || recovery_rate < 0.0 || recovery_rate >= 1.0) return -2;

    try {
        const auto discount = MakeDiscountHandle(disc_tenors, disc_rates, n_disc);
        const auto hazard_curve = BuildHazardCurve(
            cds_tenors, cds_spreads, n_cds, recovery_rate, discount);
        if (!hazard_curve) return -2;

        CdsCurveEntry entry;
        entry.hazard = hazard_curve;
        entry.recovery_rate = recovery_rate;
        return PriceCds(
            entry, discount, maturity_years, running_spread, notional, side, npv_out, fair_spread_out);
    } catch (...) {
        return -3;
    }
}

#else

extern "C" int mylib_cds_curve_create(
    const double* /*cds_tenors*/,
    const double* /*cds_spreads*/,
    int /*n_cds*/,
    double /*recovery_rate*/,
    const char* /*disc_curve_id*/,
    char* /*id_out*/,
    int /*id_cap*/
) {
    return -4;
}

extern "C" int mylib_cds_curve_hazard(
    const char* /*curve_id*/,
    double* /*hazard_times_out*/,
    double* /*hazard_rates_out*/,
    int /*max_n*/,
    int* /*n_out*/
) {
    return -4;
}

extern "C" int mylib_cds_curve_survival(
    const char* /*curve_id*/,
    const double* /*query_times*/,
    int /*n_query*/,
    double* /*survival_out*/
) {
    return -4;
}

extern "C" int mylib_cds_curve_npv(
    const char* /*curve_id*/,
    double /*maturity_years*/,
    double /*running_spread*/,
    double /*notional*/,
    int /*side*/,
    double* /*npv_out*/,
    double* /*fair_spread_out*/
) {
    return -4;
}

extern "C" int mylib_cds_curve_release(const char* /*curve_id*/) {
    return -4;
}

extern "C" void mylib_cds_curve_release_all(void) {}

extern "C" int mylib_cds_bootstrap_curve(
    const double* /*cds_tenors*/,
    const double* /*cds_spreads*/,
    int /*n_cds*/,
    double /*recovery_rate*/,
    const double* /*disc_tenors*/,
    const double* /*disc_rates*/,
    int /*n_disc*/,
    double* /*hazard_times_out*/,
    double* /*hazard_rates_out*/
) {
    return -4;
}

extern "C" int mylib_cds_survival_probs(
    const double* /*cds_tenors*/,
    const double* /*cds_spreads*/,
    int /*n_cds*/,
    double /*recovery_rate*/,
    const double* /*disc_tenors*/,
    const double* /*disc_rates*/,
    int /*n_disc*/,
    const double* /*query_times*/,
    int /*n_query*/,
    double* /*survival_out*/
) {
    return -4;
}

extern "C" int mylib_cds_npv(
    double /*maturity_years*/,
    double /*running_spread*/,
    double /*notional*/,
    int /*side*/,
    double /*recovery_rate*/,
    const double* /*cds_tenors*/,
    const double* /*cds_spreads*/,
    int /*n_cds*/,
    const double* /*disc_tenors*/,
    const double* /*disc_rates*/,
    int /*n_disc*/,
    double* /*npv_out*/,
    double* /*fair_spread_out*/
) {
    return -4;
}

#endif