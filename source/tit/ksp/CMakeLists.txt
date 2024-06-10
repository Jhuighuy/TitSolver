# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    ksp
  SOURCES
    "blas.hpp"
    "operator.hpp"
    "precond.hpp"
    "solver.hpp"
    "solver_bicgstab.hpp"
    "solver_cg.hpp"
    "solver_cgs.hpp"
    "solver_gmres.hpp"
    "solver_idrs.hpp"
    "solver_newton.hpp"
    "solver_tfqmr.hpp"
  DEPENDS
    tit::core)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    ksp_tests
  SOURCES
    "operator.test.cpp"
  DEPENDS
    tit::ksp
    tit::testing)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
