# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Test application: parses arguments, prints the resulting property tree as
# indented JSON to stdout. Covers all property system features.
add_tit_executable(
  NAME
    prop_app
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test_app.cpp"
  DEPENDS
    tit::prop
)

add_tit_test(
  NAME "service_outputs"
  COMMAND "${SHELL_EXE}" -c
    "\"$1\" --help > ./help.txt &&
     \"$1\" --help-json > ./help.json &&
     \"$1\" --init ./init.yaml"
    "sh"
    "$<TARGET_FILE:prop_app>"
  MATCH_FILES
    "service/help.txt"
    "service/help.json"
    "service/init.yaml"
)

add_tit_test(
  NAME "cli_nested_success"
  TARGET prop_app
    "--debug"
    "--workers" "8"
    "--output" "123"
    "--mode" "safe"
    "--physics.gravity" "19.62"
    "--physics.config.substeps" "5"
    "--shape._active" "rect"
    "--shape.rect.width" "4.0"
    "--shape.rect.height" "2.0"
  MATCH_STDOUT "cli_nested_success.json"
)

add_tit_test(
  NAME "config_precedence"
  INPUT_FILES
    "config/config.json"
    "config/config2.json"
  TARGET prop_app
    "--config" "config.json"
    "--config" "config2.json"
    "--mode" "debug"
    "--shape._active" "rect"
    "--shape.rect.height" "3.5"
  MATCH_STDOUT "config_precedence.json"
)

add_tit_test(
  NAME "error_surface"
  TARGET prop_app "--workers" "200"
  EXIT_CODE 1
  MATCH_STDERR "error_surface.stderr"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
