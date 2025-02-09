# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find the PNPM executable.
find_program(PNPM_EXE NAMES "pnpm" REQUIRED)

# A list of the PNPM package source file extensions. Fell free to add more.
set(
  PNPM_SOURCE_EXTENSIONS
  ".html"
  ".css"
  ".js" ".json" ".jsx" ".mjs"
  ".ts" ".tsx" ".mts"
  ".svg"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add a PNPM target.
#
function(add_pnpm_target TARGET)
  if(NOT TARGET)
    message(FATAL_ERROR "PNPM target name must be specified.")
  endif()

  # Find "package.json".
  set(TARGET_PACKAGE "${CMAKE_CURRENT_SOURCE_DIR}/package.json")
  if(NOT EXISTS ${TARGET_PACKAGE})
    message(FATAL_ERROR "'package.json' file does not exist!")
  endif()

  # Run PNPM install.
  set(TARGET_NODE_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/node_modules")
  cmake_path(
    RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
    BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE RELATIVE_SOURCE_DIR
  )
  add_custom_command(
    COMMENT
      "Installing dependencies for PNPM package ${RELATIVE_SOURCE_DIR}"
    COMMAND "${CHRONIC_EXE}" "${PNPM_EXE}" install
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${TARGET_PACKAGE}"
    OUTPUT "${TARGET_NODE_MODULES}"
  )

  # Find all the project files.
  set(TARGET_SOURCES)
  foreach(EXT ${PNPM_SOURCE_EXTENSIONS})
    file(GLOB EXT_CONFIGS "${CMAKE_CURRENT_SOURCE_DIR}/*${EXT}")
    list(APPEND TARGET_SOURCES ${EXT_CONFIGS})
  endforeach()
  foreach(EXT ${PNPM_SOURCE_EXTENSIONS})
    file(GLOB_RECURSE EXT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*${EXT}")
    list(APPEND TARGET_SOURCES ${EXT_SOURCES})
  endforeach()

  # Define the target output directory.
  set(TARGET_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/dist")

  # Run PNPM build.
  add_custom_command(
    COMMENT "Building PNPM package ${RELATIVE_SOURCE_DIR}"
    COMMAND
      "${CMAKE_COMMAND}" -E env "PNPM_OUTPUT_DIR=${TARGET_OUTPUT_DIR}"
        "${CHRONIC_EXE}" "${PNPM_EXE}" run build
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${TARGET_SOURCES}" "${TARGET_NODE_MODULES}"
    OUTPUT "${TARGET_OUTPUT_DIR}"
  )

  # Add target that would be built once the package is updated.
  add_custom_target("${TARGET}" ALL DEPENDS "${TARGET_OUTPUT_DIR}")
  set_target_properties(
    "${TARGET}"
    PROPERTIES
      "PNPM_SOURCES" "${TARGET_SOURCES}"
      "PNPM_OUTPUT_DIR" "${TARGET_OUTPUT_DIR}"
  )
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Install the PNPM target.
#
function(install_pnpm_target)
  # Parse and check arguments.
  cmake_parse_arguments(INSTALL "" "TARGET;DESTINATION" "" ${ARGN})
  if(NOT INSTALL_TARGET)
     message(FATAL_ERROR "PNPM target name must be specified.")
  endif()
  if(NOT INSTALL_DESTINATION)
    message(FATAL_ERROR "Install destination must be specified.")
  endif()

  # Get the output directory.
  get_target_property(TARGET_OUTPUT_DIR "${INSTALL_TARGET}" "PNPM_OUTPUT_DIR")
  if(NOT TARGET_OUTPUT_DIR)
    message(FATAL_ERROR "Target ${INSTALL_TARGET} is not a PNPM target.")
  endif()

  # Install the package.
  install(
    DIRECTORY "${TARGET_OUTPUT_DIR}/"
    DESTINATION "${INSTALL_DESTINATION}"
  )
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Lint the PNPM target.
#
function(lint_pnpm_target TARGET)
  if(NOT TARGET)
    message(FATAL_ERROR "PNPM target name must be specified.")
  endif()

  # Should we skip analysis?
  if(SKIP_ANALYSIS)
    return()
  endif()

  # Get the source directory and sources of the target.
  get_target_property(TARGET_SOURCES "${TARGET}" "PNPM_SOURCES")
  if(NOT TARGET_SOURCES)
    message(FATAL_ERROR "Target ${TARGET} is not a PNPM target.")
  endif()
  get_target_property(TARGET_SOURCE_DIR "${TARGET}" SOURCE_DIR)
  get_target_property(TARGET_BINARY_DIR "${TARGET}" BINARY_DIR)

  # Run the PNPM lint.
  set(STAMP "${TARGET_BINARY_DIR}/${TARGET}.pnpm_lint_stamp")
  cmake_path(
    RELATIVE_PATH TARGET_SOURCE_DIR
    BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE RELATIVE_SOURCE_DIR
  )
  add_custom_command(
    COMMENT "Linting PNPM package ${RELATIVE_SOURCE_DIR}"
    COMMAND "${CHRONIC_EXE}" "${PNPM_EXE}" run lint
    COMMAND "${CMAKE_COMMAND}" -E touch "${STAMP}"
    WORKING_DIRECTORY "${TARGET_SOURCE_DIR}"
    DEPENDS ${TARGET_SOURCES}
    OUTPUT "${STAMP}"
  )

  # Add target that would be "built" once the package is updated.
  add_custom_target("${TARGET}_lint" ALL DEPENDS "${TARGET}" "${STAMP}")
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
