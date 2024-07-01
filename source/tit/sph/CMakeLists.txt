# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    sph
  SOURCES
    "TitParticle.hpp"
    "artificial_viscosity.hpp"
    "continuity_equation.hpp"
    "density_equation.hpp"
    "energy_equation.hpp"
    "equation_of_motion.hpp"
    "equation_of_state.hpp"
    "field.hpp"
    "fluid_equations.hpp"
    "fsi.hpp"
    "gas_equations.hpp"
    "godunov.hpp"
    "kernel.hpp"
    "momentum_equation.hpp"
    "particle_array.hpp"
    "time_integrator.hpp"
    "viscosity.hpp"
  DEPENDS
    tit::core
    tit::geom
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    sph_tests
  SOURCES
  DEPENDS
    tit::sph
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
