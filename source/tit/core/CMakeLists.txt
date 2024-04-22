# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES
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
  "vec_avx.hpp"
  "vec_neon.hpp"
  "vec_simd.hpp"
  "vec.hpp")

# Create the library.
add_library(tit_core STATIC ${CXX_SOURCES})
add_library(tit::core ALIAS tit_core)

# Link with the dependent libraries.
target_link_libraries(tit_core PUBLIC tit::base)

# Enable static analysis.
enable_clang_tidy(tit::core)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
