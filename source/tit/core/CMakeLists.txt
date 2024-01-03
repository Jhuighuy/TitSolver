# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES
  "assert.hpp"
  "config.hpp"
  "exception.hpp"
  "func_utils.hpp"
  "graph.hpp"
  "mat.hpp"
  "math.hpp"
  "mdvector.hpp"
  "meta.hpp"
  "misc.hpp"
  "multivector.hpp"
  "signals.cpp"
  "signals.hpp"
  "simd.hpp"
  "stacktrace.hpp"
  "string_utils.hpp"
  "trait_utils.hpp"
  "types.hpp"
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
enable_static_analysis(tit::core)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
