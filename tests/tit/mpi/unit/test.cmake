# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Direct launch: MPI must come up as a single-process world.
add_tit_test(TARGET tit::mpi_tests)

# Launches through `mpiexec` at various process counts.
add_tit_test(NAME "np1" TARGET tit::mpi_tests MPI_RANKS 1)
add_tit_test(NAME "np2" TARGET tit::mpi_tests MPI_RANKS 2)
add_tit_test(NAME "np4" TARGET tit::mpi_tests MPI_RANKS 4)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
