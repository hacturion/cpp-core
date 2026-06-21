cmake_minimum_required(VERSION 3.20)
project(mylib_numerical LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------------
# Build options
# ------------------------------------------------------------------
option(MYLIB_BUILD_SHARED     "Build shared library (mylib.dll for Excel / ctypes)" ON)
option(MYLIB_BUILD_PYTHON     "Build Python module via pybind11 (set OFF for pure C++ XLL builds)" OFF)
option(MYLIB_USE_EIGEN        "Fetch and use Eigen (highly recommended)" ON)
option(MYLIB_USE_BOOST        "Use Boost (Sobol quasi-random via boost-random)" OFF)
option(MYLIB_USE_QUANTLIB     "Use QuantLib (yield curves; requires Boost)" OFF)
option(MYLIB_BUILD_TESTS      "Build unit tests" OFF)

# ------------------------------------------------------------------
# Core interface target (all headers)
# ------------------------------------------------------------------
add_library(mylib_core INTERFACE)
target_include_directories(mylib_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ------------------------------------------------------------------
# Eigen (header-only) - fetched automatically if enabled
# ------------------------------------------------------------------
if(MYLIB_USE_EIGEN)
    include(FetchContent)
    FetchContent_Declare(
        eigen
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG        3.4.0
    )
    FetchContent_MakeAvailable(eigen)

    target_link_libraries(mylib_core INTERFACE Eigen3::Eigen)
    target_compile_definitions(mylib_core INTERFACE MYLIB_HAS_EIGEN)
    message(STATUS "Eigen enabled for mylib")
endif()

# ------------------------------------------------------------------
# Boost (header-only Sobol via boost-random; also required by QuantLib)
# ------------------------------------------------------------------
if(MYLIB_USE_BOOST OR MYLIB_USE_QUANTLIB)
    find_package(Boost REQUIRED CONFIG COMPONENTS random)
    target_link_libraries(mylib_core INTERFACE Boost::random)
    target_compile_definitions(mylib_core INTERFACE MYLIB_HAS_BOOST)
    message(STATUS "Boost enabled for mylib")
endif()

# ------------------------------------------------------------------
# QuantLib (yield curves)
# ------------------------------------------------------------------
if(MYLIB_USE_QUANTLIB)
    if(NOT MYLIB_USE_BOOST)
        message(FATAL_ERROR "MYLIB_USE_QUANTLIB requires MYLIB_USE_BOOST")
    endif()
    find_package(QuantLib REQUIRED CONFIG)
    target_link_libraries(mylib_core INTERFACE QuantLib::QuantLib)
    target_compile_definitions(mylib_core INTERFACE MYLIB_HAS_QUANTLIB)
    message(STATUS "QuantLib enabled for mylib")
endif()

# ------------------------------------------------------------------
# Implementation library (static) - contains all the actual code
# ------------------------------------------------------------------
set(MYLIB_IMPL_SOURCES
    src/mylib.cpp
    src/random/prng.cpp
    src/random/sobol.cpp
    src/interpolation/tension_spline.cpp
)

if(MYLIB_USE_BOOST)
    list(APPEND MYLIB_IMPL_SOURCES
        src/random/boost_sobol.cpp
    )
endif()

if(MYLIB_USE_QUANTLIB)
    list(APPEND MYLIB_IMPL_SOURCES
        src/finance/yield_curve.cpp
        src/finance/cds.cpp
        src/finance/equity_derivatives.cpp
    )
endif()

add_library(mylib_impl STATIC ${MYLIB_IMPL_SOURCES})
target_link_libraries(mylib_impl PUBLIC mylib_core)
target_compile_definitions(mylib_impl PUBLIC MYLIB_STATIC)

# ------------------------------------------------------------------
# Shared library (C ABI) - consumed by Excel-DNA and ctypes
# ------------------------------------------------------------------
if(MYLIB_BUILD_SHARED)
    add_library(mylib SHARED)
    target_link_libraries(mylib PRIVATE mylib_impl)
    target_compile_definitions(mylib PRIVATE MYLIB_EXPORTS)

    # Workaround for "No SOURCES given to target" on Visual Studio
    target_sources(mylib PRIVATE $<TARGET_OBJECTS:mylib_impl>)

    set_target_properties(mylib PROPERTIES
        OUTPUT_NAME "mylib"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
endif()

# ------------------------------------------------------------------
# Python bindings (pybind11)
# ------------------------------------------------------------------
if(MYLIB_BUILD_PYTHON)
    include(FetchContent)

    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG        v2.13.6
    )
    FetchContent_MakeAvailable(pybind11)

    pybind11_add_module(mylib_python
        python/bindings.cpp
    )
    target_link_libraries(mylib_python PRIVATE mylib_impl)   # Link against static implementation (avoids DLL export issues)

    set_target_properties(mylib_python PROPERTIES 
        OUTPUT_NAME "mylib"
        ARCHIVE_OUTPUT_NAME "mylib_python")
endif()

# ------------------------------------------------------------------
# Installation
# ------------------------------------------------------------------
if(MYLIB_BUILD_SHARED)
    install(TARGETS mylib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()

install(DIRECTORY include/ DESTINATION include)
