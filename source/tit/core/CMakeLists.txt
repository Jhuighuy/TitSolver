# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    core
  SOURCES
    "basic_types.hpp"
    "checks.hpp"
    "graph.hpp"
    "io.hpp"
    "main_func.cpp"
    "main_func.hpp"
    "mat.hpp"
    "math.hpp"
    "mdvector.hpp"
    "meta.hpp"
    "multivector.hpp"
    "profiler.cpp"
    "profiler.hpp"
    "simd.hpp"
    "system.cpp"
    "system.hpp"
    "time.hpp"
    "type_traits.hpp"
    "utils.hpp"
    "vec.hpp"
    "vec_avx.hpp"
    "vec_neon.hpp"
    "vec_simd.hpp"
  DEPENDS
    tit::base)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    core_tests
  SOURCES
    "math.test.cpp"
    "meta.test.cpp"
    "mdvector.test.cpp"
    "system.test.cpp"
    "time.test.cpp"
    "vec.test.cpp"
  DEPENDS
    tit::core tit::testing)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
