#pragma once

// Main random number header for mylib
// Includes both traditional PRNGs and quasi-random (Sobol) generators.

#include "mylib/random/sobol.hpp"

#include <random>
#include <cstdint>

namespace mylib::random {

/**
 * Simple high-quality PRNG wrapper (Mersenne Twister 64-bit).
 * Good default for most Monte Carlo work when you don't need quasi-random.
 */
class PRNG {
public:
    using result_type = std::uint64_t;

    explicit PRNG(std::uint64_t seed = 42);

    std::uint64_t next_uint64();
    double next_double();           // [0, 1)

    void seed(std::uint64_t new_seed);

private:
    std::mt19937_64 engine_;
};

/**
 * Convenience function to create a Sobol generator.
 */
inline Sobol make_sobol(int dimension) {
    return Sobol(dimension);
}

} // namespace mylib::random
