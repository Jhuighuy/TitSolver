# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Exercise the real solver lifecycle and collective output on two ranks with a
# deliberately small problem.
add_tit_test(
  NAME "distributed_smoke"
  TARGET
    tit::titwcsph
    "--max-steps" "1"
    "--particles-per-height" "8"
  MPI_RANKS 2
)

# Checkpoint on one rank and resume the same state on two ranks.
add_tit_test(
  NAME "restart_different_ranks"
  COMMAND
    "${SHELL_EXE}"
    "${CMAKE_CURRENT_LIST_DIR}/restart.sh"
    "${MPIEXEC_EXECUTABLE}"
    "${MPIEXEC_NUMPROC_FLAG}"
    "$<TARGET_FILE:titwcsph>"
  FLAGS RUN_SERIAL
)

# This long-running system test currently checks successful completion. Binary
# HDF5 files are intentionally not compared byte-for-byte; run-format and
# numerical semantics are covered independently.
add_tit_test(
  NAME "dam_breaking[long]"
  COMMAND "titwcsph"
  FLAGS RUN_SERIAL
  ENVIRONMENT TIT_ENABLE_PROFILER=1
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
