# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find the PNPM executable.
find_program(PNPM_EXE NAMES "pnpm" REQUIRED)

# Setup the PNPM test runner executable.
set(PNPM_TEST_CMD "pnpm run $<IF:$<CONFIG:Coverage>,coverage,test>")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add a PNPM target.
#
function(add_tit_pnpm_target)
  # Parse and check arguments.
  cmake_parse_arguments(TARGET "" "NAME;DESTINATION" "" ${ARGN})
  make_target_name(${TARGET_NAME} TARGET)

  # Find "package.json".
  set(TARGET_PACKAGE_JSON "${CMAKE_CURRENT_SOURCE_DIR}/package.json")
  if(NOT EXISTS ${TARGET_PACKAGE_JSON})
    message(FATAL_ERROR "'package.json' file does not exist!")
  endif()

  # Find all the source files.
  set(TARGET_SOURCES)
  foreach(EXT ".html" ".css" ".svg" ".json"
              ".js" ".jsx" ".mjs" ".ts" ".tsx" ".mts")
    file(GLOB EXT_CONFIGS "${CMAKE_CURRENT_SOURCE_DIR}/*${EXT}")
    file(GLOB_RECURSE EXT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*${EXT}")
    list(APPEND TARGET_SOURCES ${EXT_CONFIGS} ${EXT_SOURCES})
  endforeach()

  # Run `pnpm install`.
  set(STAMP "${CMAKE_CURRENT_SOURCE_DIR}/node_modules/.modules.yaml")
  add_custom_command(
    COMMENT "Installing dependencies for PNPM package ${TARGET}"
    COMMAND "${CHRONIC_EXE}" "${PNPM_EXE}" install
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${TARGET_PACKAGE_JSON}"
    OUTPUT "${STAMP}"
  )
  add_custom_target("${TARGET}_install" ALL DEPENDS "${STAMP}")

  # Run `pnpm run build`.
  set(TARGET_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/dist")
  set(STAMP "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.stamp")
  add_custom_command(
    COMMENT "Building PNPM package ${TARGET}"
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${TARGET_OUTPUT_DIR}"
    COMMAND
      "${CMAKE_COMMAND}" -E env "PNPM_OUTPUT_DIR=${TARGET_OUTPUT_DIR}"
        "${CHRONIC_EXE}" "${PNPM_EXE}" run build
    COMMAND "${CMAKE_COMMAND}" -E touch "${STAMP}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS ${TARGET_SOURCES} "${TARGET_NAME}_install"
    OUTPUT "${STAMP}"
  )
  add_custom_target("${TARGET}" ALL DEPENDS "${STAMP}")

  # Run the linters.
  if(NOT SKIP_ANALYSIS)
    set(STAMP "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_lint.stamp")
    add_custom_command(
      COMMENT "Linting PNPM package ${TARGET}"
      COMMAND "${CHRONIC_EXE}" "${PNPM_EXE}" run lint
      COMMAND "${CMAKE_COMMAND}" -E touch "${STAMP}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      DEPENDS ${TARGET_SOURCES} "${TARGET_NAME}_install"
      OUTPUT "${STAMP}"
    )
    add_custom_target("${TARGET}_lint" ALL DEPENDS "${STAMP}")
  endif()

  # Install the target.
  if(TARGET_DESTINATION)
    install(
      DIRECTORY "${TARGET_OUTPUT_DIR}/"
      DESTINATION "${TARGET_DESTINATION}"
    )
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
