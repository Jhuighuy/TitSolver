# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    par
  SOURCES
    "atomic.hpp"
    "control.cpp"
    "control.hpp"
    "memory_pool.hpp"
    "task_group.hpp"
    "thread.hpp"
  DEPENDS
    tit::core
    TBB::tbb
    TBB::tbbmalloc)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    par_tests
  SOURCES
    "atomic.test.cpp"
    "control.test.cpp"
    "memory_pool.test.cpp"
    "task_group.test.cpp"
  DEPENDS
    tit::par
    tit::testing)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
