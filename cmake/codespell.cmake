# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find codespell.
find_program(CODESPELL_EXE NAMES "codespell")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Check spelling in the specified directory.
#
function(check_spelling)
  # Should we skip analysis?
  if(SKIP_ANALYSIS)
    return()
  endif()

  # Exit early in case codespell was not found.
  if(NOT CODESPELL_EXE)
    return()
  endif()

  # We'll check indexed files only. The best way to do this is to ask git
  # what files are indexed in the directory, and pass those to codespell.
  execute_process(
    COMMAND "${GIT_EXE}" ls-files
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE INDEXED_FILES
  )
  string(STRIP "${INDEXED_FILES}" INDEXED_FILES)
  string(REPLACE "\n" ";" INDEXED_FILES "${INDEXED_FILES}")
  list(TRANSFORM INDEXED_FILES STRIP)
  list(TRANSFORM INDEXED_FILES PREPEND "${CMAKE_SOURCE_DIR}/")

  # Create a stamp.
  set(STAMP "${CMAKE_BINARY_DIR}/${TARGET}.spell_stamp")

  # Run codespell.
  add_custom_command(
    COMMENT "Checking spelling in ${CMAKE_SOURCE_DIR}"
    OUTPUT "${STAMP}"
    COMMAND
      "${CODESPELL_EXE}"
        --ignore-words="${CMAKE_SOURCE_DIR}/cmake/codespell.txt"
        --check-filenames
        ${INDEXED_FILES}
    COMMAND
      "${CMAKE_COMMAND}" -E touch "${STAMP}"
    DEPENDS ${INDEXED_FILES}
  )

  # Add target that would be built once documentation files are updated.
  add_custom_target("${CMAKE_PROJECT_NAME}_codespell" ALL DEPENDS "${STAMP}")
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
