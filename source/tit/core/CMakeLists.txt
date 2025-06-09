# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_subdirectory("_cxx")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    core
  SOURCES
    "_mat/eig.hpp"
    "_mat/fact.hpp"
    "_mat/mat.hpp"
    "_mat/part.hpp"
    "_mat/traits.hpp"
    "_simd/deduce.hpp"
    "_simd/mask.hpp"
    "_simd/reg_mask.hpp"
    "_simd/reg.hpp"
    "_simd/traits.hpp"
    "_vec/traits.hpp"
    "_vec/vec_mask.hpp"
    "_vec/vec.hpp"
    "basic_types.hpp"
    "checks.cpp"
    "checks.hpp"
    "cmd.cpp"
    "cmd.hpp"
    "containers/inplace_vector.hpp"
    "containers/inplace_flat_map.hpp"
    "containers/mdvector.hpp"
    "containers/multivector.hpp"
    "containers/small_flat_map.hpp"
    "containers/small_vector.hpp"
    "enum_utils.hpp"
    "exception.cpp"
    "exception.hpp"
    "io.hpp"
    "log.hpp"
    "mat.hpp"
    "math.hpp"
    "meta.hpp"
    "numbers/dual.hpp"
    "numbers/strict.hpp"
    "par/algorithms.hpp"
    "par/atomic.hpp"
    "par/control.cpp"
    "par/control.hpp"
    "par/memory_pool.hpp"
    "par/task_group.hpp"
    "profiler.cpp"
    "profiler.hpp"
    "rand_utils.hpp"
    "range_utils.hpp"
    "serialization.hpp"
    "simd.hpp"
    "stats.cpp"
    "stats.hpp"
    "str_utils.hpp"
    "stream.hpp"
    "sys/malloc.cpp"
    "sys/signal.cpp"
    "sys/signal.hpp"
    "sys/utils.cpp"
    "sys/utils.hpp"
    "time.hpp"
    "tuple_utils.hpp"
    "type_utils.hpp"
    "uint_utils.hpp"
    "utils.hpp"
    "vec.hpp"
  DEPENDS
    _cxx
    Boost::container
    Boost::core
    gcem
    hwy::hwy
    mimalloc-static
    TBB::tbb
    TBB::tbbmalloc
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    core_tests
  SOURCES
    "_mat/eig.test.cpp"
    "_mat/fact.test.cpp"
    "_mat/mat.test.cpp"
    "_mat/part.test.cpp"
    "_simd/deduce.test.cpp"
    "_simd/mask.test.cpp"
    "_simd/reg_mask.test.cpp"
    "_simd/reg.test.cpp"
    "_vec/vec_mask.test.cpp"
    "_vec/vec.test.cpp"
    "containers/mdvector.test.cpp"
    "containers/multivector.test.cpp"
    "enum_utils.test.cpp"
    "math.test.cpp"
    "meta.test.cpp"
    "numbers/dual.test.cpp"
    "par/algorithms.test.cpp"
    "par/atomic.test.cpp"
    "par/control.test.cpp"
    "par/memory_pool.test.cpp"
    "par/task_group.test.cpp"
    "rand_utils.test.cpp"
    "serialization.test.cpp"
    "serialization.testing.hpp"
    "str_utils.test.cpp"
    "sys/signal.test.cpp"
    "sys/utils.test.cpp"
    "time.test.cpp"
    "type_utils.test.cpp"
    "uint_utils.test.cpp"
  DEPENDS
    tit::core
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
