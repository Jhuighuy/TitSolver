# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    io
  SOURCES
    "run.cpp"
    "run.hpp"
    "run_backend.hpp"
    "type.hpp"
  DEPENDS
    tit::core
    HighFive::HighFive
    nlohmann_json::nlohmann_json
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    io_tests
  SOURCES
    "run.test.cpp"
  DEPENDS
    tit::io
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    io_mpi
  SOURCES
    "parallel_run.cpp"
    "parallel_run.hpp"
  DEPENDS
    tit::dist
    tit::io
    HDF5::HDF5
    MPI::MPI_CXX
)

add_tit_executable(
  NAME
    io_mpi_tests
  SOURCES
    "parallel_run.test.cpp"
  DEPENDS
    tit::io_mpi
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
