# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find Doxygen.
find_package(Doxygen COMPONENTS doxygen)

# Configure Doxygen.
set(DOXYGEN_QUIET YES)
set(DOXYGEN_GENERATE_HTML NO)
set(DOXYGEN_GENERATE_XML YES)
set(DOXYGEN_EXTRACT_ALL YES)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add a Doxygen XML target.
#
function(add_tit_doxygen_target)
  # Parse and check arguments.
  cmake_parse_arguments(TARGET "" "NAME" "SOURCES" ${ARGN})
  if(NOT TARGET_NAME)
    message(FATAL_ERROR "Python module name must be specified.")
  endif()

  # Create the target.
  cmake_path(
    RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
    BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE RELATIVE_SOURCE_DIR
  )
  make_target_name("${TARGET_NAME}" TARGET)
  doxygen_add_docs(
    "${TARGET}"
    "${TARGET_SOURCES}"
    ALL
    USE_STAMP_FILE
    COMMENT "Running Doxygen on ${RELATIVE_SOURCE_DIR}"
  )
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
