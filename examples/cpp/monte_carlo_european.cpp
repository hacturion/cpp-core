// examples/cpp/monte_carlo_european.cpp
//
// Tiny example: European call option priced by Monte Carlo.
// This demonstrates how the random number generators + accurate summation
// will be used in real pricing code.

#include "mylib/random.hpp"
#include "mylib.h"   // for mylib_sum (Kahan)

#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>

namespace mylib::examples {

double european_call_mc(
    double S0,
    double K,
    double r,
    double sigma,
    double T,
    int n_paths,
    int n_steps,
    bool use_sobol = false
) {
    using namespace mylib::random;

    const double dt = T / n_steps;
    const double drift = (r - 0.5 * sigma * sigma) * dt;
    const double vol = sigma * std::sqrt(dt);

    std::vector<double> payoffs;
    payoffs.reserve(n_paths);

    if (use_sobol) {
        Sobol sobol(1); // 1D for simplicity in this toy example

        for (int p = 0; p < n_paths; ++p) {
            double z = sobol.next()[0];
            // Convert uniform [0,1) to standard normal (Box-Muller simplified)
            double u2 = (p + 0.5) / n_paths; // crude second uniform
            double z1 = std::sqrt(-2.0 * std::log(z + 1e-12)) * std::cos(2 * std::acos(-1.0) * u2);

            double ST = S0 * std::exp(drift * n_steps + vol * std::sqrt(n_steps) * z1);
            double payoff = std::max(ST - K, 0.0);
            payoffs.push_back(std::exp(-r * T) * payoff);
        }
    } else {
        PRNG rng(42);

        for (int p = 0; p < n_paths; ++p) {
            double z = rng.next_double();
            double u2 = rng.next_double();
            double z1 = std::sqrt(-2.0 * std::log(z + 1e-12)) * std::cos(2 * std::acos(-1.0) * u2);

            double ST = S0 * std::exp(drift * n_steps + vol * std::sqrt(n_steps) * z1);
            double payoff = std::max(ST - K, 0.0);
            payoffs.push_back(std::exp(-r * T) * payoff);
        }
    }

    // Use Kahan summation from the core library for better accuracy
    return mylib_sum(payoffs.data(), static_cast<int>(payoffs.size()));
}

} // namespace mylib::examples

// Standalone demo
int main() {
    using namespace mylib::examples;

    double price = european_call_mc(
        100.0,   // S0
        100.0,   // K
        0.05,    // r
        0.2,     // sigma
        1.0,     // T
        100000,  // n_paths
        1        // n_steps (Euler)
    );

    std::cout << "MC European Call price (PRNG): " << price << "\n";

    double price_sobol = european_call_mc(100.0, 100.0, 0.05, 0.2, 1.0, 100000, 1, true);
    std::cout << "MC European Call price (Sobol): " << price_sobol << "\n";

    return 0;
}
