# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find codespell.
find_program(
  CODESPELL_EXE
  NAMES "codespell"
)

# Find list of words for codespell to ignore.
if(CODESPELL_EXE)
  find_file(
    CODESPELL_IGNORE_PATH
    NAMES "codespell.txt"
    PATHS "${CMAKE_SOURCE_DIR}/cmake"
    REQUIRED
  )
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

##
## Check spelling in the specified directory.
##
## Unlike other static analysis tools, I do not want to run spellcheck pear each
## target separately. There two issues: first, codespell is written in Python,
## and it's startup and execution times can be better, so it will slow down our
## build; second, I want spell checking in all indexed files, not just source
## files.
##
function(check_spelling TARGET)
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
    COMMAND ${GIT_EXE} ls-files "${CMAKE_CURRENT_SOURCE_DIR}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE INDEXED_FILES
  )
  string(REPLACE "\n" ";" INDEXED_FILES "${INDEXED_FILES}")

  # Create a stamp.
  set(STAMP "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.spell_stamp")

  # Run codespell.
  add_custom_command(
    COMMENT "Checking spelling in ${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT "${STAMP}"
    COMMAND
      "${CODESPELL_EXE}"
        --ignore-words="${CODESPELL_IGNORE_PATH}"
        --check-filenames
        ${INDEXED_FILES}
    COMMAND
      "${CMAKE_COMMAND}" -E touch "${STAMP}"
    DEPENDS ${INDEXED_FILES}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" # Codespell uses relative paths.
  )

  # Add target that would be built once documentation files are updated.
  add_custom_target("${TARGET}_codespell" ALL DEPENDS "${STAMP}")
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
