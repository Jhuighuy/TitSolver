# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    par
  SOURCES
    "atomic.hpp"
    "memory_pool.hpp"
    "thread.hpp"
  DEPENDS
    tit::core)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    par_tests
  SOURCES
    "memory_pool.test.cpp"
  DEPENDS
    tit::par tit::testing)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
