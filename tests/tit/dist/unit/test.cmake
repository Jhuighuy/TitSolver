# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Direct launch: the exchange must degenerate gracefully to a single process.
add_tit_test(TARGET tit::dist_tests)

# Launches through `mpiexec` at various process counts.
add_tit_test(NAME "np2" TARGET tit::dist_tests MPI_RANKS 2)
add_tit_test(NAME "np4" TARGET tit::dist_tests MPI_RANKS 4)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
