# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

tit_add_library(
  NAME
    sph
  SOURCES
    artificial_viscosity.hpp
    density_equation.hpp
    equation_of_state.hpp
    field.hpp
    fluid_equations.hpp
    fsi.hpp
    gas_equations.hpp
    kernel.hpp
    time_integrator.hpp
    TitParticle.hpp
  DEPENDS
    core par geom)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

tit_add_executable(
  NAME
    sph_tests
  SOURCES
  DEPENDS
    sph testing)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
