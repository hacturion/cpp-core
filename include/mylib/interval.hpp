#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <ostream>

namespace mylib {

/**
 * Basic interval arithmetic (skeleton).
 *
 * This is a starting point for rigorous numerical methods and validated solvers.
 * It is intentionally simple and header-only for now.
 *
 * Future improvements:
 * - Directed rounding (using std::fenv)
 * - More operations (pow, exp, log, etc.)
 * - Interval Newton, Hansen-Sengupta, etc.
 */
template <typename T = double>
class Interval {
public:
    T lower;
    T upper;

    Interval() : lower(0), upper(0) {}
    Interval(T value) : lower(value), upper(value) {}
    Interval(T lo, T hi) : lower(std::min(lo, hi)), upper(std::max(lo, hi)) {}

    static Interval empty() {
        return {std::numeric_limits<T>::max(), std::numeric_limits<T>::lowest()};
    }

    static Interval entire() {
        return {std::numeric_limits<T>::lowest(), std::numeric_limits<T>::max()};
    }

    bool contains(T x) const { return x >= lower && x <= upper; }
    bool is_empty() const { return lower > upper; }
    T width() const { return upper - lower; }
    T mid() const { return (lower + upper) / T(2); }

    // Basic arithmetic
    friend Interval operator+(const Interval& a, const Interval& b) {
        return {a.lower + b.lower, a.upper + b.upper};
    }

    friend Interval operator-(const Interval& a, const Interval& b) {
        return {a.lower - b.upper, a.upper - b.lower};
    }

    friend Interval operator*(const Interval& a, const Interval& b) {
        T c1 = a.lower * b.lower;
        T c2 = a.lower * b.upper;
        T c3 = a.upper * b.lower;
        T c4 = a.upper * b.upper;
        return {std::min({c1, c2, c3, c4}), std::max({c1, c2, c3, c4})};
    }

    friend Interval operator/(const Interval& a, const Interval& b) {
        // Simplified division (does not handle zero crossing perfectly yet)
        if (b.lower <= 0 && b.upper >= 0) {
            return Interval::entire(); // division by interval containing zero
        }
        T c1 = a.lower / b.lower;
        T c2 = a.lower / b.upper;
        T c3 = a.upper / b.lower;
        T c4 = a.upper / b.upper;
        return {std::min({c1, c2, c3, c4}), std::max({c1, c2, c3, c4})};
    }

    friend std::ostream& operator<<(std::ostream& os, const Interval& iv) {
        os << "[" << iv.lower << ", " << iv.upper << "]";
        return os;
    }
};

using Intervald = Interval<double>;

} // namespace mylib
