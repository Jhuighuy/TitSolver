# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES
  "artificial_viscosity.hpp"
  "density_equation.hpp"
  "equation_of_state.hpp"
  "field.hpp"
  "fluid_equations.hpp"
  "fsi.hpp"
  "gas_equations.hpp"
  "kernel.hpp"
  "time_integrator.hpp"
  "TitParticle.hpp")

# Create the library.
add_library(tit_sph INTERFACE ${CXX_SOURCES})
add_library(tit::sph ALIAS tit_sph)

# Link with the dependent libraries.
target_link_libraries(tit_sph INTERFACE tit::core tit::geom tit::par)

# Enable static analysis.
enable_clang_tidy(tit::sph)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
