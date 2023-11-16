# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES
  "assert.hpp"
  "config.hpp"
  "graph.hpp"
  "mat.hpp"
  "math.hpp"
  "mdvector.hpp"
  "meta.hpp"
  "misc.hpp"
  "multivector.hpp"
  "pool_allocator.hpp"
  "simd.hpp"
  "types.hpp"
  "vec_avx.hpp"
  "vec_neon.hpp"
  "vec_simd.hpp"
  "vec.hpp")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Create the library.
add_library(tit_core INTERFACE ${CXX_SOURCES})
add_library(tit::core ALIAS tit_core)

# Link with the dependant libraries.
target_link_libraries(tit_core INTERFACE tit::base)

# Enable static analysis.
check_includes(tit::core)
enable_clang_tidy(tit::core)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
