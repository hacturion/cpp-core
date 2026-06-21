#include "mylib.h"

#include "discount_curve.hpp"

#include <cmath>

#ifdef MYLIB_HAS_QUANTLIB

#include <ql/exercise.hpp>
#include <ql/instruments/bonds/convertiblebonds.hpp>
#include <ql/instruments/europeanoption.hpp>
#include <ql/instruments/payoffs.hpp>
#include <ql/methods/lattices/binomialtree.hpp>
#include <ql/pricingengines/bond/binomialconvertibleengine.hpp>
#include <ql/pricingengines/vanilla/analyticeuropeanengine.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/settings.hpp>
#include <ql/termstructures/volatility/equityfx/blackconstantvol.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/time/calendars/unitedstates.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>

namespace {

constexpr int kUnknownCurve = -5;

QuantLib::Calendar EquityCalendar() {
    return QuantLib::UnitedStates(QuantLib::UnitedStates::NYSE);
}

QuantLib::Date MaturityFromYears(const QuantLib::Date& settlement, double years) {
    const int months = std::max(1, static_cast<int>(std::lround(years * 12.0)));
    return EquityCalendar().advance(settlement, QuantLib::Period(months, QuantLib::Months));
}

bool ValidPositive(double x) {
    return std::isfinite(x) && x > 0.0;
}

QuantLib::ext::shared_ptr<QuantLib::BlackScholesMertonProcess> MakeEquityProcess(
    double spot,
    double volatility,
    double dividend_yield,
    const QuantLib::Handle<QuantLib::YieldTermStructure>& risk_free
) {
    if (!ValidPositive(spot) || !ValidPositive(volatility) || dividend_yield < 0.0 || risk_free.empty()) {
        return {};
    }

    const QuantLib::Date ref = risk_free->referenceDate();
    const QuantLib::Calendar calendar = EquityCalendar();
    const QuantLib::DayCounter dayCounter = QuantLib::Actual365Fixed();

    const auto spotQuote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(spot);
    const auto divCurve = QuantLib::ext::make_shared<QuantLib::FlatForward>(
        ref, dividend_yield, dayCounter, QuantLib::Continuous, QuantLib::Annual);
    const auto volCurve = QuantLib::ext::make_shared<QuantLib::BlackConstantVol>(
        ref, calendar, volatility, dayCounter);

    return QuantLib::ext::make_shared<QuantLib::BlackScholesMertonProcess>(
        QuantLib::Handle<QuantLib::Quote>(spotQuote),
        QuantLib::Handle<QuantLib::YieldTermStructure>(divCurve),
        risk_free,
        QuantLib::Handle<QuantLib::BlackVolTermStructure>(volCurve));
}

} // namespace

extern "C" int mylib_equity_option_npv(
    double spot,
    double strike,
    double volatility,
    double maturity_years,
    const char* disc_curve_id,
    int option_type,
    double dividend_yield,
    double* npv_out
) {
    if (!disc_curve_id || !disc_curve_id[0] || !npv_out) return -1;
    if (!ValidPositive(spot) || !ValidPositive(strike) || !ValidPositive(volatility) ||
        !ValidPositive(maturity_years) || dividend_yield < 0.0 ||
        (option_type != 1 && option_type != -1)) {
        return -2;
    }

    try {
        mylib::finance::SetEvaluationDate();
        const auto riskFree = mylib::finance::LookupZeroCurve(disc_curve_id);
        if (riskFree.empty()) return kUnknownCurve;

        const auto process = MakeEquityProcess(spot, volatility, dividend_yield, riskFree);
        if (!process) return -2;

        const QuantLib::Date settlement = QuantLib::Settings::instance().evaluationDate();
        const QuantLib::Date maturity = MaturityFromYears(settlement, maturity_years);
        if (maturity <= settlement) return -2;

        const auto payoff = QuantLib::ext::make_shared<QuantLib::PlainVanillaPayoff>(
            option_type > 0 ? QuantLib::Option::Call : QuantLib::Option::Put,
            strike);
        const auto exercise = QuantLib::ext::make_shared<QuantLib::EuropeanExercise>(maturity);
        QuantLib::EuropeanOption option(payoff, exercise);
        option.setPricingEngine(QuantLib::ext::make_shared<QuantLib::AnalyticEuropeanEngine>(process));

        *npv_out = option.NPV();
        return std::isfinite(*npv_out) ? 0 : -3;
    } catch (...) {
        return -3;
    }
}

extern "C" int mylib_convertible_bond_npv(
    double spot,
    double face_value,
    double coupon_rate,
    double maturity_years,
    double conversion_ratio,
    double volatility,
    double credit_spread,
    const char* disc_curve_id,
    double dividend_yield,
    double* npv_out
) {
    if (!disc_curve_id || !disc_curve_id[0] || !npv_out) return -1;
    if (!ValidPositive(spot) || !ValidPositive(face_value) || !ValidPositive(maturity_years) ||
        !ValidPositive(conversion_ratio) || !ValidPositive(volatility) ||
        credit_spread < 0.0 || coupon_rate < 0.0 || dividend_yield < 0.0) {
        return -2;
    }

    try {
        mylib::finance::SetEvaluationDate();
        const auto riskFree = mylib::finance::LookupZeroCurve(disc_curve_id);
        if (riskFree.empty()) return kUnknownCurve;

        const auto process = MakeEquityProcess(spot, volatility, dividend_yield, riskFree);
        if (!process) return -2;

        const QuantLib::Calendar calendar = EquityCalendar();
        const QuantLib::Date settlement = QuantLib::Settings::instance().evaluationDate();
        const QuantLib::Date maturity = MaturityFromYears(settlement, maturity_years);
        if (maturity <= settlement) return -2;

        const QuantLib::DayCounter dayCounter = QuantLib::Actual365Fixed();
        const QuantLib::Schedule schedule(
            settlement,
            maturity,
            QuantLib::Period(QuantLib::Semiannual),
            calendar,
            QuantLib::ModifiedFollowing,
            QuantLib::ModifiedFollowing,
            QuantLib::DateGeneration::Backward,
            false);

        const std::size_t nCoupons = schedule.size() > 1 ? schedule.size() - 1 : 1;
        const QuantLib::Rate periodCoupon = coupon_rate / 2.0;
        std::vector<QuantLib::Rate> coupons(nCoupons, periodCoupon);

        const auto exercise = QuantLib::ext::make_shared<QuantLib::EuropeanExercise>(maturity);
        const QuantLib::CallabilitySchedule callability;

        QuantLib::ConvertibleFixedCouponBond bond(
            exercise,
            conversion_ratio,
            callability,
            settlement,
            2,
            coupons,
            dayCounter,
            schedule,
            face_value);

        const auto spreadQuote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(credit_spread);
        constexpr QuantLib::Integer kTreeSteps = 256;
        bond.setPricingEngine(QuantLib::ext::make_shared<QuantLib::BinomialConvertibleEngine<QuantLib::CoxRossRubinstein>>(
            process,
            kTreeSteps,
            QuantLib::Handle<QuantLib::Quote>(spreadQuote)));

        *npv_out = bond.NPV();
        return std::isfinite(*npv_out) ? 0 : -3;
    } catch (...) {
        return -3;
    }
}

#else

extern "C" int mylib_equity_option_npv(
    double /*spot*/,
    double /*strike*/,
    double /*volatility*/,
    double /*maturity_years*/,
    const char* /*disc_curve_id*/,
    int /*option_type*/,
    double /*dividend_yield*/,
    double* /*npv_out*/
) {
    return -4;
}

extern "C" int mylib_convertible_bond_npv(
    double /*spot*/,
    double /*face_value*/,
    double /*coupon_rate*/,
    double /*maturity_years*/,
    double /*conversion_ratio*/,
    double /*volatility*/,
    double /*credit_spread*/,
    const char* /*disc_curve_id*/,
    double /*dividend_yield*/,
    double* /*npv_out*/
) {
    return -4;
}

#endif