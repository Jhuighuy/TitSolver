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
    "equation_of_state.hpp"
    "distributed_particles.hpp"
    "field.hpp"
    "fluid_equations.hpp"
    "kernel.hpp"
    "kernel.inl.hpp"
    "particle_array.hpp"
    "particle_mesh.hpp"
    "simulation.hpp"
    "time_integrator.hpp"
  DEPENDS
    tit::core
    tit::dist
    tit::geom
    tit::par
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    sph_tests
  SOURCES
    "fluid_equations.test.cpp"
    "kernel.test.cpp"
    "particle_array.test.cpp"
    "particle_mesh.test.cpp"
  DEPENDS
    tit::sph
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    sph_mpi_tests
  SOURCES
    "simulation.mpi.test.cpp"
  DEPENDS
    tit::sph
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
