#pragma once

#include <cstdint>
#include <vector>
#include <stdexcept>

namespace mylib::random {

/**
 * High-quality Sobol sequence generator (quasi-Monte Carlo).
 *
 * This implementation uses the standard Gray-code algorithm and direction
 * numbers based on the tables from Stephen Joe and Frances Kuo
 * (http://web.maths.unsw.edu.au/~fkuo/sobol/).
 *
 * It is suitable for production Monte Carlo work in finance.
 *
 * Features:
 * - Up to 40 dimensions (easily extendable)
 * - 64-bit generation
 * - Skip-ahead support (important for variance reduction / splitting)
 */
class Sobol {
public:
    /**
     * Construct a Sobol generator.
     *
     * @param dimension   Number of dimensions (1 to 40 currently supported)
     * @param seed        Ignored for Sobol (deterministic), kept for API compatibility
     */
    explicit Sobol(int dimension = 1);

    /// Returns the next point in the unit hypercube [0,1)^d as a vector of doubles.
    std::vector<double> next();

    /// Fills an existing buffer with the next point (avoids allocation).
    void next(double* out, int dim);

    /// Skip ahead by n points (very useful for parallelization and variance reduction).
    void skip(std::uint64_t n);

    /// Current index in the sequence (0-based).
    std::uint64_t index() const { return index_; }

    int dimension() const { return dim_; }

    /// Reset to the beginning of the sequence.
    void reset();

private:
    int dim_;
    std::uint64_t index_{0};

    // Internal state
    std::vector<std::uint64_t> x_;           // current state per dimension
    std::vector<std::uint64_t> direction_;   // direction numbers (flattened)

public:
    static constexpr int MAX_DIM = 40;

private:
    // Precomputed direction numbers (from Joe & Kuo tables)
    static const std::uint32_t direction_numbers_[MAX_DIM][63];
};

} // namespace mylib::random
