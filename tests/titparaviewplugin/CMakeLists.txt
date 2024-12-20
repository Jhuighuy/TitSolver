# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

find_program(COVERAGE_PY_EXE NAMES "coverage")

add_tit_test(
  NAME "titparaviewplugin/export_csv"
  INPUT_FILES "input.ttdb"
  MATCH_FILES "output.csv.checksum"
  COMMAND
    "${COVERAGE_PY_EXE}"
    run
    "--data-file=${CMAKE_SOURCE_DIR}/.coverage"
    "${CMAKE_SOURCE_DIR}/output/TIT_ROOT/paraview_plugin/titdata.py"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
