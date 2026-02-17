# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    malloc
  SOURCES
    "malloc.cpp"
  DEPENDS
    mimalloc-static
)

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
    "checks.hpp"
    "env.cpp"
    "env.hpp"
    "exception.cpp"
    "exception.hpp"
    "main.cpp"
    "main.hpp"
    "mat.hpp"
    "math.hpp"
    "mdvector.hpp"
    "posix.cpp"
    "posix.hpp"
    "print.cpp"
    "print.hpp"
    "profiler.cpp"
    "profiler.hpp"
    "range.hpp"
    "serialization.hpp"
    "simd.hpp"
    "stacktrace.hpp"
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
    "zip.cpp"
    "zip.hpp"
    "zstd.cpp"
    "zstd.hpp"
  DEPENDS
    $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:tit::malloc>
    gcem
    hwy::hwy
    miniz::miniz
    nlohmann_json::nlohmann_json
    zstd::libzstd
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
    "env.test.cpp"
    "math.test.cpp"
    "mdvector.test.cpp"
    "posix.test.cpp"
    "serialization.test.cpp"
    "str.test.cpp"
    "time.test.cpp"
    "type.test.cpp"
    "zstd.test.cpp"
  DEPENDS
    tit::core
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
