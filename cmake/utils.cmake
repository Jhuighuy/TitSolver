# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find shell.
find_program(SHELL_EXE NAMES "bash" "zsh" "sh" REQUIRED)

# Find xargs.
find_program(XARGS_EXE NAMES "xargs" REQUIRED)

# Find git.
find_program(GIT_EXE NAMES "git" REQUIRED)

# Find chronic.
find_program(CHRONIC_EXE
  NAMES "chronic.sh"
  PATHS "${CMAKE_SOURCE_DIR}/build"
  REQUIRED
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Get a selected subset of the target's compile options: include directories
# and compile definitions.
#
macro(get_generated_compile_options TARGET OPTIONS_VAR)
  # Append include directories.
  foreach(PROP INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES)
    set(TARGET_INCLUDE_DIRS "$<TARGET_PROPERTY:${TARGET},${PROP}>")
    set(TARGET_INCLUDE_DIRS "$<LIST:SORT,${TARGET_INCLUDE_DIRS}>")
    set(TARGET_INCLUDE_DIRS "$<REMOVE_DUPLICATES:${TARGET_INCLUDE_DIRS}>")
    list(APPEND ${OPTIONS_VAR}
      "$<LIST:TRANSFORM,${TARGET_INCLUDE_DIRS},PREPEND,-I>"
    )
  endforeach()

  # Append compile definitions.
  foreach(PROP COMPILE_DEFINITIONS INTERFACE_COMPILE_DEFINITIONS)
    set(TARGET_DEFS "$<TARGET_PROPERTY:${TARGET},${PROP}>")
    set(TARGET_DEFS "$<LIST:SORT,${TARGET_DEFS}>")
    set(TARGET_DEFS "$<REMOVE_DUPLICATES:${TARGET_DEFS}>")
    set(TARGET_DEFS "$<FILTER:${TARGET_DEFS},INCLUDE,.+>") # remove stray `;`.
    list(APPEND ${OPTIONS_VAR}
      "$<LIST:TRANSFORM,${TARGET_DEFS},PREPEND,-D>"
    )
  endforeach()
endmacro()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Install each file in the list.
#
function(install_each_file)
  # Parse and check arguments.
  cmake_parse_arguments(INSTALL "" "DESTINATION;BASE_DIR" "FILES" ${ARGN})
  if(NOT INSTALL_DESTINATION)
    message(FATAL_ERROR "Installation destination must be specified.")
  endif()
  if(NOT INSTALL_BASE_DIR)
    message(FATAL_ERROR "Base directory must be specified.")
  endif()

  # Install the files.
  foreach(FILE ${INSTALL_FILES})
    # Get the directory of the file relative to the output directory,
    # and append it to the installation destination. This is needed to
    # properly handle the hierarchy of the files, otherwise CMake will
    # install all the files in the destination directory in a flat manner.
    get_filename_component(FILE_DIR "${FILE}" DIRECTORY)
    cmake_path(
      RELATIVE_PATH FILE_DIR
      BASE_DIRECTORY "${INSTALL_BASE_DIR}"
      OUTPUT_VARIABLE FILE_DESTINATION_DIR
    )
    if(FILE_DESTINATION_DIR STREQUAL ".")
      set(FILE_DESTINATION_DIR "") # Just to provide a cleaner output.
    endif()
    set(FILE_DESTINATION_DIR "${INSTALL_DESTINATION}/${FILE_DESTINATION_DIR}")

    # Install the file to the destination directory.
    install(FILES "${FILE}" DESTINATION "${FILE_DESTINATION_DIR}")
  endforeach()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
