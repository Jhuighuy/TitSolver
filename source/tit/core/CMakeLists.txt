# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

configure_file("build_info.cpp.in" "build_info.cpp" @ONLY)

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
    "build_info.cpp"
    "build_info.hpp"
    "checks.cpp"
    "checks.hpp"
    "containers/inplace_vector.hpp"
    "containers/mdvector.hpp"
    "containers/str_hash_map.hpp"
    "containers/str_hash_set.hpp"
    "env.cpp"
    "env.hpp"
    "exception.hpp"
    "mat.hpp"
    "math.hpp"
    "print.cpp"
    "print.hpp"
    "profiler.cpp"
    "profiler.hpp"
    "range.hpp"
    "serialization.hpp"
    "simd.hpp"
    "stats.cpp"
    "stats.hpp"
    "str.cpp"
    "str.hpp"
    "stream.hpp"
    "sys_info-linux.cpp"
    "sys_info-macos.cpp"
    "sys_info.cpp"
    "sys_info.hpp"
    "time.hpp"
    "type.cpp"
    "type.hpp"
    "utils.hpp"
    "vec.hpp"
  DEPENDS
    Boost::container
    gcem
    hwy::hwy
    nlohmann_json::nlohmann_json
    TBB::tbb
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
    "env.test.cpp"
    "math.test.cpp"
    "serialization.test.cpp"
    "str.test.cpp"
    "time.test.cpp"
    "type.test.cpp"
  DEPENDS
    tit::core
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
