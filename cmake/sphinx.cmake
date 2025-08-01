# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
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
  cmake_parse_arguments(TARGET "" "NAME;DESTINATION" "DEPENDS" ${ARGN})
  if(NOT TARGET_NAME)
    message(FATAL_ERROR "Target name must be specified.")
  endif()

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

  # Parse the dependencies. For each Doxygen dependency, configure the Breathe
  # options to point to the Doxygen XML output directory.
  cmake_parse_arguments(TARGET_DEPS "" "" "AUTODOC;DOXYGEN" ${TARGET_DEPENDS})
  set(BREATHE_OPTIONS)
  foreach(DEP ${TARGET_DEPS_DOXYGEN})
    get_target_property(DEP_DIR "${DEP}" BINARY_DIR)
    list(APPEND BREATHE_OPTIONS "-Dbreathe_projects.${DEP}=${DEP_DIR}/xml")
  endforeach()

  # Define the list of the dependency files. For each Autodoc targets, we
  # collect all the Python sources. For each Doxygen targets, we collect the
  # stamps files produced by the CMake target.
  #
  # NOTE: For some reason, rebuilding of Sphinx target is not triggered unless
  #       we explicitly depend on both Doxygen target and its stamp file.
  set(TARGET_DEP_FILES)
  foreach(DEP ${TARGET_DEPS_AUTODOC})
    get_target_property(DEP_SOURCE_DIR "${DEP}" SOURCE_DIR)
    file(GLOB_RECURSE DEP_SOURCES "${DEP_SOURCE_DIR}/*.py")
    list(APPEND TARGET_DEP_FILES ${DEP_SOURCES})
  endforeach()
  foreach(DEP ${TARGET_DEPS_DOXYGEN})
    get_target_property(DEP_DIR "${DEP}" BINARY_DIR)
    list(APPEND TARGET_DEP_FILES "${DEP}" "${DEP_DIR}/${DEP}.stamp")
  endforeach()

  # Run sphinx-build.
  cmake_path(
    RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
    BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE RELATIVE_SOURCE_DIR
  )
  add_custom_command(
    COMMENT "Building Sphinx target in ${RELATIVE_SOURCE_DIR}"
    OUTPUT ${TARGET_OUTPUT}
    COMMAND
      "${CMAKE_COMMAND}"
        -E env "TZ=UTC" # Sphinx may fail if TZ is not set.
        "${SPHINX_EXE}"
          ${SPHINX_OPTIONS}
          ${BREATHE_OPTIONS}
          "${CMAKE_CURRENT_SOURCE_DIR}"
          "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS
      "${TARGET_CONFIG}"
      ${TARGET_PAGES}
      ${TARGET_STATIC_FILES}
      ${TARGET_TEMPLATES}
      ${TARGET_DEP_FILES}
  )

  # Create the target.
  make_target_name(${TARGET_NAME} TARGET)
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
