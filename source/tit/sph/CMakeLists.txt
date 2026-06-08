# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    sph_kernel_generator
  SOURCES
    "kernel.gen.cpp"
  DEPENDS
    tit::core
    symengine::symengine
)

add_tit_codegen(
  NAME
    sph_kernel_codegen
  GENERATOR
    sph_kernel_generator
  OUTPUT
    "kernel.inl.hpp"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    sph
  SOURCES
    "boundary_extrapolation.hpp"
    "buoyancy.hpp"
    "equation_of_state.hpp"
    "field.hpp"
    "fluid_equations.hpp"
    "kernel.hpp"
    "kernel.inl.hpp"
    "particle_array.hpp"
    "particle_mesh.hpp"
    "time_integrator.hpp"
    "turbulence_model.hpp"
    "viscosity.hpp"
    "wall_model.hpp"
  DEPENDS
    tit::core
    tit::data
    tit::geom
    tit::par
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    sph_tests
  SOURCES
    "boundary_extrapolation.test.cpp"
    "kernel.test.cpp"
    "wall_model.test.cpp"
  DEPENDS
    tit::sph
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
