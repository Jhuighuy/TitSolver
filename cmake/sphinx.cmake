# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find the Sphinx executable.
find_program(SPHINX_BUILD_EXE NAMES "sphinx-build" REQUIRED)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add a Sphinx documentation target.
#
function(add_sphinx_target TARGET)
  if(NOT TARGET)
    message(FATAL_ERROR "Sphinx target name must be specified.")
  endif()

  # Find all the source files.
  set(TARGET_SOURCES)
  set(TARGET_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/conf.py")
  if(NOT EXISTS "${TARGET_CONFIG}")
    message(FATAL_ERROR "Sphinx configuration file does not exist!")
  endif()
  list(APPEND TARGET_SOURCES "${TARGET_CONFIG}")
  file(GLOB_RECURSE TARGET_PAGES "${CMAKE_CURRENT_SOURCE_DIR}/*.rst")
  list(APPEND TARGET_SOURCES ${TARGET_PAGES})
  file(GLOB_RECURSE TARGET_STATIC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/_static/*")
  list(APPEND TARGET_SOURCES ${TARGET_STATIC_FILES})
  file(GLOB_RECURSE TARGET_TEMPLATES "${CMAKE_CURRENT_SOURCE_DIR}/_templates/*")
  list(APPEND TARGET_SOURCES ${TARGET_TEMPLATES})

  # Define the list of the output files. Pages are compiled to HTML files,
  # while static files are copied to the output directory. Templates are
  # used to generate the HTML files, therefore they are part of the output.
  #
  # Note that what we are defining here is not the exact list of the output
  # files produced by the Sphinx build. Templates that we are using do not
  # utilize many of the Sphinx features, like, for example, the search index.
  # Unfortunately, Sphinx cannot be configured to not produce the search index,
  # but since our HTML pages do not link to the search index, we can safely
  # ignore it.
  set(TARGET_OUTPUT)
  set(TARGET_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/build")
  foreach(PAGE ${TARGET_PAGES})
    # Skip pages that start with underscore, those are meant to be hidden.
    if("${PAGE}" MATCHES "(.+/)?_.*\\.rst$")
      continue()
    endif()
    cmake_path(RELATIVE_PATH PAGE OUTPUT_VARIABLE PAGE_NAME)
    string(REPLACE ".rst" ".html" PAGE_NAME "${PAGE_NAME}")
    list(APPEND TARGET_OUTPUT "${TARGET_OUTPUT_DIR}/${PAGE_NAME}")
  endforeach()
  foreach(FILE ${TARGET_STATIC_FILES})
    cmake_path(RELATIVE_PATH FILE OUTPUT_VARIABLE FILE_NAME)
    list(APPEND TARGET_OUTPUT "${TARGET_OUTPUT_DIR}/${FILE_NAME}")
  endforeach()

  # Setup sphinx-build arguments.
  set(
    SPHINX_BUILD_ARGS
    --quiet           # Be quiet.
    --nitpicky        # Warn about missing references.
    --fail-on-warning # Warnings are errors.
    --builder html    # Build only the HTML output.
    --write-all       # Sphinx sometimes fails to detect changes.
  )

  # Run sphinx-build.
  cmake_path(
    RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
    BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE RELATIVE_SOURCE_DIR
  )
  add_custom_command(
    COMMENT "Building Sphinx target ${RELATIVE_SOURCE_DIR}"
    OUTPUT ${TARGET_OUTPUT}
    COMMAND
      "${CMAKE_COMMAND}"
        -E env "TZ=UTC" # Sphinx may fail if TZ is not set.
        "${SPHINX_BUILD_EXE}"
          ${SPHINX_BUILD_ARGS}
          "${CMAKE_CURRENT_SOURCE_DIR}"
          "${TARGET_OUTPUT_DIR}"
    DEPENDS ${TARGET_SOURCES}
  )

  # Add target that would be built once documentation files are updated.
  add_custom_target("${TARGET}" ALL DEPENDS ${TARGET_OUTPUT})
  set_target_properties(
    "${TARGET}"
    PROPERTIES
      "SPHINX_OUTPUT" "${TARGET_OUTPUT}"
      "SPHINX_OUTPUT_DIR" "${TARGET_OUTPUT_DIR}"
  )
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Install the Sphinx target.
#
function(install_sphinx_target)
  # Parse and check arguments.
  cmake_parse_arguments(INSTALL "" "TARGET;DESTINATION" "" ${ARGN})
  if(NOT INSTALL_TARGET)
     message(FATAL_ERROR "Sphinx target name must be specified.")
  endif()
  if(NOT INSTALL_DESTINATION)
    message(FATAL_ERROR "Install destination must be specified.")
  endif()

  # Get the output files.
  get_target_property(TARGET_OUTPUT "${INSTALL_TARGET}" "SPHINX_OUTPUT")
  get_target_property(TARGET_OUTPUT_DIR "${INSTALL_TARGET}" "SPHINX_OUTPUT_DIR")
  if(NOT TARGET_OUTPUT OR NOT TARGET_OUTPUT_DIR)
    message(FATAL_ERROR "Target ${INSTALL_TARGET} is not a Sphinx target.")
  endif()

  # Install the documentation.
  foreach(FILE ${TARGET_OUTPUT})
    # Get the directory of the file relative to the output directory,
    # and append it to the installation destination. This is needed to
    # properly handle the hierarchy of the files, otherwise CMake will
    # install all the files in the destination directory in a flat manner.
    get_filename_component(FILE_DIR "${FILE}" DIRECTORY)
    cmake_path(
      RELATIVE_PATH FILE_DIR
      BASE_DIRECTORY "${TARGET_OUTPUT_DIR}"
      OUTPUT_VARIABLE FILE_DESTINATION_DIR
    )
    if(FILE_DESTINATION_DIR STREQUAL ".")
      set(FILE_DESTINATION_DIR "") # Just to provide a cleaner output.
    endif()
    set(FILE_DESTINATION_DIR "${INSTALL_DESTINATION}/${FILE_DESTINATION_DIR}")
    install(FILES "${FILE}" DESTINATION "${FILE_DESTINATION_DIR}")
  endforeach()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
