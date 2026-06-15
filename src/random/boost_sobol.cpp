#include "mylib.h"

#include <cstdint>
#include <limits>

#ifdef MYLIB_HAS_BOOST
#include <boost/random/sobol.hpp>
#include <boost/random/uniform_real.hpp> // boost::uniform_real (wraps uniform_real_distribution)
#endif

#ifdef MYLIB_HAS_BOOST

namespace {

constexpr int kMaxDim = 3667;

} // namespace

extern "C" int mylib_sobol_points(
    int dimension,
    int n_points,
    std::uint64_t skip,
    double* out
) {
    if (!out || dimension <= 0 || n_points <= 0) return -1;
    if (dimension > kMaxDim) return -2;

    try {
        boost::random::sobol engine(static_cast<std::size_t>(dimension));
        boost::uniform_real<double> dist;

        if (skip > 0) {
            engine.discard(static_cast<boost::uintmax_t>(skip) * static_cast<std::size_t>(dimension));
        }

        for (int i = 0; i < n_points; ++i) {
            for (int d = 0; d < dimension; ++d) {
                out[static_cast<size_t>(i) * dimension + d] = dist(engine);
            }
        }
        return 0;
    } catch (...) {
        return -3;
    }
}

#else

extern "C" int mylib_sobol_points(
    int /*dimension*/,
    int /*n_points*/,
    std::uint64_t /*skip*/,
    double* /*out*/
) {
    return -4;
}

#endif