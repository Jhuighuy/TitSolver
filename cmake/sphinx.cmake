# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find the Sphinx executable.
find_program(SPHINX_BUILD_EXE NAMES "sphinx-build" REQUIRED)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

##
## Add a Sphinx documentation target.
##
function(add_sphinx_target)
  # Parse and check arguments.
  cmake_parse_arguments(SPHINX "" "NAME" "" ${ARGN})
  if(NOT SPHINX_NAME)
    message(FATAL_ERROR "Sphinx target name must be specified.")
  endif()

  # Check that the configuration file exists.
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/conf.py")
    message(FATAL_ERROR "Sphinx configuration file does not exist!")
  endif()

  # Find all source files: `*.rst` files for documentation,
  # `*.html`, `*.css`, `*.js` and `*.svg` files for the theme.
  set(SPHINX_SOURCES "conf.py")
  foreach(EXT ".rst" ".html" ".css" ".js" ".svg")
    file(GLOB_RECURSE FILES "${CMAKE_CURRENT_SOURCE_DIR}/*${EXT}")
    list(APPEND SPHINX_SOURCES ${FILES})
  endforeach()

  # Create a stamp.
  set(STAMP "${CMAKE_CURRENT_BINARY_DIR}/${SPHINX_NAME}.sphinx_stamp")

  # Create the output directory.
  set(SPHINX_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/sphinx_output")

  # Run sphinx-build.
  add_custom_command(
    COMMENT "Building Sphinx target in ${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT "${STAMP}"
    COMMAND
      "${CMAKE_COMMAND}"
        -E env "TZ=UTC" # Sphinx may fail if TZ is not set.
        "${SPHINX_BUILD_EXE}"
          --quiet
          --builder html
          --write-all # Sphinx sometimes fails to detect changes.
          "${CMAKE_CURRENT_SOURCE_DIR}" "${SPHINX_OUTPUT_DIR}"
    COMMAND
      "${CMAKE_COMMAND}" -E touch "${STAMP}"
    DEPENDS ${SPHINX_SOURCES}
  )

  # Add target that would be built once documentation files are updated.
  add_custom_target("${SPHINX_NAME}" ALL DEPENDS "${STAMP}")

  # Set the output directory property.
  set_target_properties(
    "${SPHINX_NAME}"
    PROPERTIES "SPHINX_OUTPUT_DIR" "${SPHINX_OUTPUT_DIR}"
  )
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

##
## Install the Sphinx target.
##
function(install_sphinx_target)
  # Parse and check arguments.
  cmake_parse_arguments(SPHINX "" "TARGET;DESTINATION" "" ${ARGN})
  if(NOT SPHINX_TARGET)
    message(FATAL_ERROR "Sphinx target name must be specified.")
  endif()
  if(NOT SPHINX_DESTINATION)
    message(FATAL_ERROR "Sphinx destination must be specified.")
  endif()

  # Get the output directory.
  get_target_property(SPHINX_OUTPUT_DIR "${SPHINX_TARGET}" "SPHINX_OUTPUT_DIR")
  if(NOT SPHINX_OUTPUT_DIR)
    message(
      FATAL_ERROR
      "Sphinx output directory was not set for the target ${SPHINX_TARGET}. "
      "Is it a Sphinx target?"
    )
  endif()

  # Install the documentation.
  install(
    DIRECTORY "${SPHINX_OUTPUT_DIR}/" # Trailing slash is important.
    DESTINATION "${SPHINX_DESTINATION}"
    # Exclude build cache and other unnecessary files.
    PATTERN ".buildinfo" EXCLUDE
    PATTERN ".doctrees" EXCLUDE
    PATTERN "objects.inv" EXCLUDE
  )
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
