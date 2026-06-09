// python/bindings.cpp
// Modern pybind11 bindings for the numerical C++ library.
// This gives you proper NumPy support, exceptions, docstrings, etc.
//
// Build with CMake (MYLIB_BUILD_PYTHON=ON).

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <cmath>

#include "mylib.h"
#include "mylib/interval.hpp"
#include "mylib/random.hpp"

#include "bindings_random.cpp"   // temporary include for development

// Note: The Monte Carlo demo is currently in examples/cpp/.
// We will expose it properly once the library structure settles.

namespace py = pybind11;

// Helper to convert pybind11 array to raw pointer safely
static void check_array(const py::array_t<double>& arr, const char* name) {
    if (arr.ndim() != 1) {
        throw std::runtime_error(std::string(name) + " must be 1-dimensional");
    }
}

PYBIND11_MODULE(mylib, m) {
    m.doc() = "Numerical methods core library (C++ backend)";

    m.attr("__version__") = mylib_version();

    // --- Reductions (numerically careful) ---
    m.def("sum", [](py::array_t<double> arr) {
        check_array(arr, "arr");
        auto buf = arr.request();
        return mylib_sum(static_cast<const double*>(buf.ptr), static_cast<int>(buf.shape[0]));
    }, "Sum of a 1D array using Kahan compensated summation (higher accuracy)",
       py::arg("arr"));

    m.def("weighted_sum", [](py::array_t<double> values, py::array_t<double> weights) {
        check_array(values, "values");
        check_array(weights, "weights");
        if (values.size() != weights.size()) {
            throw std::runtime_error("values and weights must have the same length");
        }
        auto vbuf = values.request();
        auto wbuf = weights.request();

        return mylib_weighted_sum(
            static_cast<const double*>(vbuf.ptr),
            static_cast<const double*>(wbuf.ptr),
            static_cast<int>(vbuf.shape[0])
        );
    }, "Weighted sum: sum(values[i] * weights[i]) using compensated summation.\n"
       "Very useful for discounted payoffs in pricing.",
       py::arg("values"), py::arg("weights"));

    // Simple scalar ops (for parity with C ABI / Excel NATIVE.ADD etc.)
    m.def("add", &mylib_add, "Add two doubles (C++ implementation)", py::arg("a"), py::arg("b"));
    m.def("multiply", &mylib_multiply, "Multiply two doubles (C++ implementation)", py::arg("a"), py::arg("b"));

    m.def("add_arrays", [](py::array_t<double> a, py::array_t<double> b) {
        check_array(a, "a");
        check_array(b, "b");
        if (a.size() != b.size()) {
            throw std::runtime_error("Arrays must have the same length");
        }

        auto buf_a = a.request();
        auto buf_b = b.request();

        py::array_t<double> result(a.size());
        auto buf_res = result.request();

        mylib_add_arrays(
            static_cast<double*>(buf_a.ptr),
            static_cast<double*>(buf_b.ptr),
            static_cast<double*>(buf_res.ptr),
            static_cast<int>(buf_a.shape[0])
        );
        return result;
    }, "Element-wise add two arrays (returns new NumPy array)",
       py::arg("a"), py::arg("b"));

    m.def("moving_average", [](py::array_t<double> input, int window) {
        check_array(input, "input");
        if (window <= 0) {
            throw std::runtime_error("window must be positive");
        }

        auto buf = input.request();
        py::array_t<double> output(input.size());
        auto buf_out = output.request();

        mylib_moving_average(
            static_cast<double*>(buf.ptr),
            static_cast<double*>(buf_out.ptr),
            static_cast<int>(buf.shape[0]),
            window
        );
        return output;
    }, "Simple moving average (returns new NumPy array)",
       py::arg("input"), py::arg("window") = 3);

    m.def("basic_stats", [](py::array_t<double> data) -> py::tuple {
        check_array(data, "data");
        auto buf = data.request();

        std::array<double, 4> stats{};
        int rc = mylib_basic_stats(
            static_cast<double*>(buf.ptr),
            static_cast<int>(buf.shape[0]),
            stats.data()
        );

        if (rc != 0) {
            throw std::runtime_error("basic_stats failed");
        }

        return py::make_tuple(stats[0], stats[1], stats[2], stats[3]);
    }, "Returns (min, max, mean, stddev) as a tuple",
       py::arg("data"));

    // --- Random numbers ---
    register_random(m);

    // --- Interval arithmetic (skeleton) ---
    py::class_<mylib::Intervald>(m, "Interval")
        .def(py::init<double>())
        .def(py::init<double, double>())
        .def_readwrite("lower", &mylib::Intervald::lower)
        .def_readwrite("upper", &mylib::Intervald::upper)
        .def("mid", &mylib::Intervald::mid)
        .def("width", &mylib::Intervald::width)
        .def("contains", &mylib::Intervald::contains)
        .def("__repr__", [](const mylib::Intervald& iv) {
            return "[" + std::to_string(iv.lower) + ", " + std::to_string(iv.upper) + "]";
        });

    // Monte Carlo demo lives in examples/cpp/monte_carlo_european.cpp for now.
    // We will wire it into the Python module in the next iteration.
    // Basic exposed version for immediate use (uses Sobol when use_sobol=true).
    m.def("european_call_mc", [](double S0, double K, double r, double sigma, double T,
                                 int n_paths, int n_steps, bool use_sobol = false) -> double {
        // Minimal implementation using the library's random + Kahan sum for demonstration.
        // For production, prefer the full example or move the logic into src/.
        using namespace mylib::random;

        const double dt = T / n_steps;
        const double drift = (r - 0.5 * sigma * sigma) * dt;
        const double vol = sigma * std::sqrt(dt);

        std::vector<double> payoffs;
        payoffs.reserve(n_paths);

        if (use_sobol) {
            Sobol sobol(1);
            for (int p = 0; p < n_paths; ++p) {
                double z = sobol.next()[0];
                double u2 = (p + 0.5) / n_paths;
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

        return mylib_sum(payoffs.data(), static_cast<int>(payoffs.size()));
    },
    "Basic European call Monte Carlo using the library's PRNG/Sobol + Kahan summation.\n"
    "For a more complete version see examples/cpp/monte_carlo_european.cpp",
    py::arg("S0"), py::arg("K"), py::arg("r"), py::arg("sigma"), py::arg("T"),
    py::arg("n_paths"), py::arg("n_steps"), py::arg("use_sobol") = false);
}
