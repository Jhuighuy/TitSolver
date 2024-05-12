# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    core
  SOURCES
    "basic_types.hpp"
    "checks.cpp"
    "checks.hpp"
    "enum_utils.hpp"
    "graph.hpp"
    "io.hpp"
    "main_func.cpp"
    "main_func.hpp"
    "mat.hpp"
    "mat/mat.hpp"
    "mat/eig.hpp"
    "mat/fact.hpp"
    "mat/part.hpp"
    "mat/traits.hpp"
    "math.hpp"
    "mdvector.hpp"
    "meta.hpp"
    "multivector.hpp"
    "profiler.cpp"
    "profiler.hpp"
    "system.cpp"
    "system.hpp"
    "time.hpp"
    "type_traits.hpp"
    "uint_utils.hpp"
    "utils.hpp"
    "vec.hpp"
    "vec/vec.hpp"
    "vec/vec_mask.hpp"
  DEPENDS
    tit::base)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    core_tests
  SOURCES
    "enum_utils.test.cpp"
    "mat/eig.test.cpp"
    "mat/fact.test.cpp"
    "mat/mat.test.cpp"
    "mat/mat.testing.hpp"
    "mat/part.test.cpp"
    "math.test.cpp"
    "meta.test.cpp"
    "mdvector.test.cpp"
    "system.test.cpp"
    "time.test.cpp"
    "uint_utils.test.cpp"
    "vec/vec.test.cpp"
    "vec/vec_mask.test.cpp"
  DEPENDS
    tit::core
    tit::testing)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
