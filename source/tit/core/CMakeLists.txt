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
    "containers/boost.hpp"
    "containers/mdvector.hpp"
    "containers/multivector.hpp"
    "enum.hpp"
    "exception.cpp"
    "exception.hpp"
    "main.cpp"
    "main.hpp"
    "mat.hpp"
    "math.hpp"
    "meta.hpp"
    "mimalloc.cpp"
    "numbers/dual.hpp"
    "numbers/strict.hpp"
    "par/algorithms.hpp"
    "par/atomic.hpp"
    "par/control.cpp"
    "par/control.hpp"
    "par/memory_pool.hpp"
    "par/task_group.hpp"
    "print.hpp"
    "profiler.cpp"
    "profiler.hpp"
    "rand.hpp"
    "range.hpp"
    "serialization.hpp"
    "signal.cpp"
    "signal.hpp"
    "simd.hpp"
    "stats.cpp"
    "stats.hpp"
    "str.hpp"
    "stream.hpp"
    "sys.cpp"
    "sys.hpp"
    "time.hpp"
    "tuple.hpp"
    "type.hpp"
    "uint.hpp"
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
    "enum.test.cpp"
    "math.test.cpp"
    "meta.test.cpp"
    "numbers/dual.test.cpp"
    "par/algorithms.test.cpp"
    "par/atomic.test.cpp"
    "par/control.test.cpp"
    "par/memory_pool.test.cpp"
    "par/task_group.test.cpp"
    "rand.test.cpp"
    "serialization.test.cpp"
    "serialization.testing.hpp"
    "signal.test.cpp"
    "str.test.cpp"
    "sys.test.cpp"
    "time.test.cpp"
    "type.test.cpp"
    "uint.test.cpp"
  DEPENDS
    tit::core
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
