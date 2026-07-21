# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

get_target_property(GUI_SOURCE_DIR titgui SOURCE_DIR)

# Unit tests (vitest). The Coverage configuration produces an lcov report at
# "source/titgui/coverage/lcov.info" for Codecov and Sonar.
add_tit_test(
  NAME "unit"
  COMMAND
    "${SHELL_EXE}" -c
    "cd '${GUI_SOURCE_DIR}' && '${NPM_EXE}' run $<IF:$<CONFIG:Coverage>,coverage,test>"
  FLAGS RUN_SERIAL
)

# End-to-end smoke test (Playwright driving the real Electron app). Needs a
# display: run as-is on macOS, under xvfb on Linux (skip when unavailable).
if(APPLE)
  set(E2E_PREFIX "")
else()
  find_program(XVFB_RUN_EXE NAMES "xvfb-run")
  if(XVFB_RUN_EXE)
    set(E2E_PREFIX "'${XVFB_RUN_EXE}' -a ")
  else()
    set(E2E_PREFIX "SKIP")
  endif()
endif()
if(NOT E2E_PREFIX STREQUAL "SKIP")
  add_tit_test(
    NAME "e2e"
    COMMAND
      "${SHELL_EXE}" -c
      "cd '${GUI_SOURCE_DIR}' && ${E2E_PREFIX}'${NPX_EXE}' playwright test"
    FLAGS RUN_SERIAL
  )
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
