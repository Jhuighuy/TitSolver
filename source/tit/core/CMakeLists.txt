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
    "exception.cpp"
    "exception.hpp"
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
    "meta/list.hpp"
    "meta/range.hpp"
    "meta/set.hpp"
    "meta/type.hpp"
    "multivector.hpp"
    "par.hpp"
    "par/atomic.hpp"
    "par/control.hpp"
    "par/control.cpp"
    "par/memory_pool.hpp"
    "par/task_group.hpp"
    "par/thread.hpp"
    "profiler.cpp"
    "profiler.hpp"
    "simd.hpp"
    "simd/deduce.hpp"
    "simd/mask.hpp"
    "simd/reg.hpp"
    "simd/reg_mask.hpp"
    "simd/traits.hpp"
    "string_utils.hpp"
    "system.cpp"
    "system.hpp"
    "time.hpp"
    "type_traits.hpp"
    "uint_utils.hpp"
    "utils.hpp"
    "vec.hpp"
    "vec/traits.hpp"
    "vec/vec.hpp"
    "vec/vec_mask.hpp"
  DEPENDS
    tit::base
    hwy::hwy
    TBB::tbb
    TBB::tbbmalloc
)

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
    "meta/list.test.cpp"
    "meta/range.test.cpp"
    "meta/set.test.cpp"
    "meta/type.test.cpp"
    "mdvector.test.cpp"
    "par/atomic.test.cpp"
    "par/control.test.cpp"
    "par/memory_pool.test.cpp"
    "par/task_group.test.cpp"
    "par/thread.test.cpp"
    "simd/deduce.test.cpp"
    "simd/mask.test.cpp"
    "simd/reg.test.cpp"
    "simd/reg_mask.test.cpp"
    "system.test.cpp"
    "time.test.cpp"
    "type_traits.test.cpp"
    "uint_utils.test.cpp"
    "vec/vec.test.cpp"
    "vec/vec_mask.test.cpp"
  DEPENDS
    tit::core
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
