# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(git)

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

  # We'll need a stamp file to track the analysis.
  set(STAMP "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.spell_stamp")

  # Run codespell.
  add_custom_command(
    COMMENT "Checking spelling in ${CMAKE_SOURCE_DIR}"
    OUTPUT "${STAMP}"
    COMMAND
      "${CODESPELL_EXE}"
        --ignore-words="${CMAKE_SOURCE_DIR}/cmake/codespell.txt"
        --check-filenames
        ${GIT_INDEXED_FILES}
    COMMAND
      "${CMAKE_COMMAND}" -E touch "${STAMP}"
    DEPENDS ${GIT_INDEXED_FILES}
  )

  # Create a custom target that should "build" once the check succeeds.
  add_custom_target("${CMAKE_PROJECT_NAME}_codespell" ALL DEPENDS "${STAMP}")
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
