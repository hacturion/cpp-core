#include "mylib.h"

#include "discount_curve.hpp"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef MYLIB_HAS_QUANTLIB
#include <ql/settings.hpp>
#endif

#ifdef MYLIB_HAS_QUANTLIB

namespace {

using ZeroCurve = mylib::finance::DiscountCurve;

constexpr int kUnknownCurve = -5;
constexpr size_t kMaxCachedCurves = 1024;

struct ZeroCurveEntry {
    std::vector<double> tenors;
    std::vector<double> rates;
};

std::mutex g_registry_mutex;
std::unordered_map<std::string, ZeroCurveEntry> g_registry;
std::atomic<std::uint64_t> g_next_curve_id{1};

bool WriteZeroCurveId(char* id_out, int id_cap, std::uint64_t id_number) {
    if (!id_out || id_cap <= 0) return false;
    const int written = std::snprintf(id_out, static_cast<size_t>(id_cap), "ZC-%08llu",
                                      static_cast<unsigned long long>(id_number));
    return written > 0 && written < id_cap;
}

const ZeroCurveEntry* FindCurve(const char* curve_id) {
    if (!curve_id || !curve_id[0]) return nullptr;
    const auto it = g_registry.find(curve_id);
    return it == g_registry.end() ? nullptr : &it->second;
}

QuantLib::Handle<QuantLib::YieldTermStructure> BuildHandleFromEntry(const ZeroCurveEntry& entry) {
    if (entry.tenors.size() < 2 || entry.tenors.size() != entry.rates.size()) {
        return {};
    }

    mylib::finance::SetEvaluationDate();
    const auto built = mylib::finance::BuildDiscountCurve(
        entry.tenors.data(),
        entry.rates.data(),
        static_cast<int>(entry.tenors.size()));
    if (!built) return {};

    auto shared = QuantLib::ext::make_shared<ZeroCurve>(*built);
    shared->enableExtrapolation();
    return QuantLib::Handle<QuantLib::YieldTermStructure>(shared);
}

} // namespace

namespace mylib::finance {

QuantLib::Handle<QuantLib::YieldTermStructure> LookupZeroCurve(const char* curve_id) {
    if (!curve_id || !curve_id[0]) return {};

    std::lock_guard<std::mutex> lock(g_registry_mutex);
    const ZeroCurveEntry* entry = FindCurve(curve_id);
    if (!entry) return {};
    return BuildHandleFromEntry(*entry);
}

} // namespace mylib::finance

extern "C" int mylib_zero_curve_create(
    const double* tenors,
    const double* zero_rates,
    int n_pillars,
    char* id_out,
    int id_cap
) {
    if (!tenors || !zero_rates || !id_out || id_cap < 12 || n_pillars < 2) {
        return -1;
    }

    try {
        mylib::finance::SetEvaluationDate();
        const auto built = mylib::finance::BuildDiscountCurve(tenors, zero_rates, n_pillars);
        if (!built) return -2;

        std::lock_guard<std::mutex> lock(g_registry_mutex);
        if (g_registry.size() >= kMaxCachedCurves) return -3;

        const std::uint64_t id_number = g_next_curve_id.fetch_add(1);
        if (!WriteZeroCurveId(id_out, id_cap, id_number)) return -1;

        ZeroCurveEntry entry;
        entry.tenors.assign(tenors, tenors + n_pillars);
        entry.rates.assign(zero_rates, zero_rates + n_pillars);
        g_registry.emplace(id_out, std::move(entry));
        return 0;
    } catch (...) {
        return -3;
    }
}

extern "C" int mylib_zero_curve_discount(
    const char* curve_id,
    double t,
    double* df_out
) {
    if (!curve_id || !df_out || t < 0.0) return -1;

    try {
        std::lock_guard<std::mutex> lock(g_registry_mutex);
        const ZeroCurveEntry* entry = FindCurve(curve_id);
        if (!entry) return kUnknownCurve;

        const auto handle = BuildHandleFromEntry(*entry);
        if (handle.empty()) return -3;

        *df_out = handle->discount(t, true);
        return std::isfinite(*df_out) ? 0 : -3;
    } catch (...) {
        return -3;
    }
}

extern "C" int mylib_zero_curve_discounts(
    const char* curve_id,
    const double* times,
    int n_times,
    double* dfs_out
) {
    if (!curve_id || !times || !dfs_out || n_times <= 0) return -1;

    try {
        std::lock_guard<std::mutex> lock(g_registry_mutex);
        const ZeroCurveEntry* entry = FindCurve(curve_id);
        if (!entry) return kUnknownCurve;

        const auto handle = BuildHandleFromEntry(*entry);
        if (handle.empty()) return -3;

        for (int i = 0; i < n_times; ++i) {
            if (times[i] < 0.0) return -2;
            dfs_out[i] = handle->discount(times[i], true);
            if (!std::isfinite(dfs_out[i])) return -3;
        }
        return 0;
    } catch (...) {
        return -3;
    }
}

extern "C" int mylib_zero_curve_rate(
    const char* curve_id,
    double t,
    double* rate_out
) {
    if (!curve_id || !rate_out || t < 0.0) return -1;

    try {
        std::lock_guard<std::mutex> lock(g_registry_mutex);
        const ZeroCurveEntry* entry = FindCurve(curve_id);
        if (!entry) return kUnknownCurve;

        const auto handle = BuildHandleFromEntry(*entry);
        if (handle.empty()) return -3;

        *rate_out = handle->zeroRate(t, QuantLib::Continuous, QuantLib::Annual, true);
        return std::isfinite(*rate_out) ? 0 : -3;
    } catch (...) {
        return -3;
    }
}

extern "C" int mylib_zero_curve_release(const char* curve_id) {
    if (!curve_id || !curve_id[0]) return -1;

    std::lock_guard<std::mutex> lock(g_registry_mutex);
    return g_registry.erase(curve_id) > 0 ? 0 : kUnknownCurve;
}

extern "C" void mylib_zero_curve_release_all(void) {
    std::lock_guard<std::mutex> lock(g_registry_mutex);
    g_registry.clear();
}

#else

extern "C" int mylib_zero_curve_create(
    const double* /*tenors*/,
    const double* /*zero_rates*/,
    int /*n_pillars*/,
    char* /*id_out*/,
    int /*id_cap*/
) {
    return -4;
}

extern "C" int mylib_zero_curve_discount(
    const char* /*curve_id*/,
    double /*t*/,
    double* /*df_out*/
) {
    return -4;
}

extern "C" int mylib_zero_curve_discounts(
    const char* /*curve_id*/,
    const double* /*times*/,
    int /*n_times*/,
    double* /*dfs_out*/
) {
    return -4;
}

extern "C" int mylib_zero_curve_rate(
    const char* /*curve_id*/,
    double /*t*/,
    double* /*rate_out*/
) {
    return -4;
}

extern "C" int mylib_zero_curve_release(const char* /*curve_id*/) {
    return -4;
}

extern "C" void mylib_zero_curve_release_all(void) {}

#endif