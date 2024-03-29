# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES
  "atomic.hpp"
  "memory_pool.hpp"
  #"thread_omp.hpp"
  #"thread_tbb.hpp"
  "thread.hpp")

# Create the library.
add_library(tit_par INTERFACE ${CXX_SOURCES})
add_library(tit::par ALIAS tit_par)

# Link with the dependent libraries.
target_link_libraries(tit_par INTERFACE tit::core)

# Enable static analysis.
enable_clang_tidy(tit::par)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
