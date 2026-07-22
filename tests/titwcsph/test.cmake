# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# This long-running system test currently checks successful completion. The
# binary HDF5 files are intentionally not compared byte-for-byte; the run-format
# and numerical semantics are covered independently.
add_tit_test(
  NAME "dam_breaking[long]"
  COMMAND "titwcsph"
  FLAGS RUN_SERIAL
  ENVIRONMENT TIT_ENABLE_PROFILER=1
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
