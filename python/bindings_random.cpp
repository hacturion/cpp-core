// python/bindings_random.cpp
// Random number bindings (will be included from main bindings later)

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "mylib/random.hpp"

namespace py = pybind11;
using namespace mylib::random;

void register_random(py::module& m) {
    py::module rng = m.def_submodule("random", "Random number generators (PRNG + Sobol)");

    // PRNG
    py::class_<PRNG>(rng, "PRNG")
        .def(py::init<std::uint64_t>(), py::arg("seed") = 42)
        .def("next_double", &PRNG::next_double, "Uniform [0, 1)")
        .def("next_uint64", &PRNG::next_uint64)
        .def("seed", &PRNG::seed);

    // Sobol (high quality quasi-random)
    py::class_<Sobol>(rng, "Sobol")
        .def(py::init<int>(), py::arg("dimension") = 1)
        .def("next", py::overload_cast<>(&Sobol::next), "Return next point as list of doubles")
        .def("skip", &Sobol::skip, "Skip ahead n points (very useful)")
        .def("reset", &Sobol::reset)
        .def_property_readonly("dimension", &Sobol::dimension)
        .def_property_readonly("index", &Sobol::index);

    // Convenience
    rng.def("make_sobol", &make_sobol, "Create a Sobol generator", py::arg("dimension"));
}
