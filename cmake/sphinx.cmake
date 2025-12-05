# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find the Sphinx executable.
find_program(SPHINX_EXE NAMES "sphinx-build" REQUIRED)

# Setup sphinx-build options.
set(
  SPHINX_OPTIONS
  "--quiet"           # Be quiet.
  "--nitpicky"        # Warn about missing references.
  "--fail-on-warning" # Warnings are errors.
  "--builder" "html"  # Build only the HTML output.
  "--write-all"       # Sphinx sometimes fails to detect changes.
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add a Sphinx target.
#
function(add_tit_sphinx_target)
  # Parse and check arguments.
  cmake_parse_arguments(TARGET "" "" "NAME;DESTINATION" ${ARGN})
  if(NOT TARGET_NAME)
    message(FATAL_ERROR "Target name must be specified.")
  endif()
  make_target_name(${TARGET_NAME} TARGET)

  # Find all the source files.
  set(TARGET_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/conf.py")
  if(NOT EXISTS "${TARGET_CONFIG}")
    message(FATAL_ERROR "Sphinx configuration file does not exist!")
  endif()
  file(GLOB_RECURSE TARGET_PAGES "${CMAKE_CURRENT_SOURCE_DIR}/*.rst")
  file(GLOB_RECURSE TARGET_STATIC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/_static/*")
  file(GLOB_RECURSE TARGET_TEMPLATES "${CMAKE_CURRENT_SOURCE_DIR}/_templates/*")

  # Define the list of the output files. Pages are compiled to HTML files,
  # while static files are copied to the output directory. Templates are
  # used to generate the HTML files, therefore they are not part of the output.
  #
  # Note that what we are defining here is not the exact list of the output
  # files produced by the Sphinx build. Templates that we are using do not
  # utilize many of the Sphinx features, like, for example, the search index.
  # Unfortunately, Sphinx cannot be configured to not produce the search index,
  # but since our HTML pages do not link to the search index, we can safely
  # ignore it.
  set(TARGET_OUTPUT)
  foreach(PAGE ${TARGET_PAGES})
    # Skip pages that start with underscore, those are meant to be hidden.
    if("${PAGE}" MATCHES "(.+/)?_.*\\.rst$")
      continue()
    endif()
    cmake_path(RELATIVE_PATH PAGE)
    string(REPLACE ".rst" ".html" PAGE "${PAGE}")
    list(APPEND TARGET_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PAGE}")
  endforeach()
  foreach(FILE ${TARGET_STATIC_FILES})
    cmake_path(RELATIVE_PATH FILE)
    list(APPEND TARGET_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FILE}")
  endforeach()

  # Run sphinx-build.
  add_custom_command(
    COMMENT "Building Sphinx target ${TARGET}"
    OUTPUT ${TARGET_OUTPUT}
    COMMAND
      "${CMAKE_COMMAND}"
        -E env "TZ=UTC" # Sphinx may fail if TZ is not set.
        "${SPHINX_EXE}"
          ${SPHINX_OPTIONS}
          "${CMAKE_CURRENT_SOURCE_DIR}"
          "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS
      "${TARGET_CONFIG}"
      ${TARGET_PAGES}
      ${TARGET_STATIC_FILES}
      ${TARGET_TEMPLATES}
  )
  add_custom_target("${TARGET}" ALL DEPENDS ${TARGET_OUTPUT})

  # Install the target.
  if(TARGET_DESTINATION)
    install_each_file(
      DESTINATION "${TARGET_DESTINATION}"
      BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}"
      FILES ${TARGET_OUTPUT}
    )
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
