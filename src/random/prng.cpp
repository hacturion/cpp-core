#include "mylib/random.hpp"

namespace mylib::random {

PRNG::PRNG(std::uint64_t seed) : engine_(seed) {}

std::uint64_t PRNG::next_uint64() {
    return engine_();
}

double PRNG::next_double() {
    // 53 bits is enough for double precision
    return static_cast<double>(engine_() >> 11) * (1.0 / (1ULL << 53));
}

void PRNG::seed(std::uint64_t new_seed) {
    engine_.seed(new_seed);
}

} // namespace mylib::random
