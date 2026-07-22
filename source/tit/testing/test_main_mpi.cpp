/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define DOCTEST_CONFIG_IMPLEMENT
#include "tit/mpi/mpi.hpp"
#include "tit/testing/test.hpp"

// Test entry point for the MPI-enabled test executables.
//
// Every process runs the full test suite, but only the main process reports:
// the reports of the remaining processes would interleave, and the test
// driver matches the output of the whole job. Failures on any process are
// still propagated through the exit code, which `mpiexec` in turn propagates
// to the job's exit code.
auto main(int argc, char** argv) -> int {
  const tit::mpi::Runtime runtime{argc, argv};
  doctest::Context context{argc, argv};
  if (!tit::mpi::world.is_main()) context.setOption("out", "/dev/null");
  return context.run();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
