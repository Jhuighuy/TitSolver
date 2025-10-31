# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# This is just a stub for normal testing, since the whole executable in it's
# actual stage is nothing more than a test itself. This test exists with a sole
# reason of not breaking anything while doing some deep refactoring in the
# library core.
add_tit_test(
  NAME "dam_breaking[long]"
  COMMAND "titwcsph"
  MATCH_FILES "particles.ttdb.checksum"
  FLAGS RUN_SERIAL
  ENVIRONMENT TIT_ENABLE_PROFILER=1
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
