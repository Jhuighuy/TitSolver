# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# This is just a stub for normal testing, since the whole executable in it's
# actual stage is nothing more than a test itself. This test exists with a sole
# reason of not breaking anything while doing some deep refactoring in the
# library core.
#
# The solver output is deterministic only when each process runs a single
# thread: with multiple threads the floating-point reduction order depends on
# the TBB scheduling. The checksum tests are therefore pinned to one thread
# per process; multi-threaded runs are validated by the physics, not by the
# bits.
add_tit_test(
  NAME "dam_breaking[long]"
  COMMAND "titwcsph"
  MATCH_FILES "particles.ttdb.checksum"
  FLAGS RUN_SERIAL
  ENVIRONMENT TIT_ENABLE_PROFILER=1 TIT_NUM_THREADS=1
)

# The same simulation distributed over two processes. The result is not
# bitwise-identical to the single-process run (the pair sums are split
# differently across the subdomains), but it is deterministic on its own.
add_tit_test(
  NAME "dam_breaking_np2[long]"
  COMMAND "titwcsph"
  MPI_RANKS 2
  MATCH_FILES "np2/particles.ttdb.checksum"
  FLAGS RUN_SERIAL
  ENVIRONMENT TIT_NUM_THREADS=1
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
