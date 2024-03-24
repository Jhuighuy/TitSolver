# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

tit_add_library(
  NAME
    core
  SOURCES
    basic_types.hpp
    checks.hpp
    graph.hpp
    io.hpp
    main_func.cpp
    main_func.hpp
    mat.hpp
    math.hpp
    mdvector.hpp
    meta.hpp
    multivector.hpp
    profiler.cpp
    profiler.hpp
    simd.hpp
    simd/fwd.hpp
    simd/reg.arm.hpp
    simd/reg.x86.hpp
    system.cpp
    system.hpp
    time.hpp
    type_traits.hpp
    uint_utils.hpp
    utils.hpp
    vec.hpp
    vec/vec.hpp
    vec/vec_mask.hpp
  DEPENDS
    base)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

tit_add_executable(
  NAME
    core_tests
  SOURCES
    math.test.cpp
    meta.test.cpp
    mdvector.test.cpp
    system.test.cpp
    time.test.cpp
    uint_utils.test.cpp
    vec/vec.test.cpp
    vec/vec.test.simd.cpp
    vec/vec.testing.hpp
    vec/vec_mask.test.cpp
    vec/vec_mask.test.simd.cpp
  DEPENDS
    core testing)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
