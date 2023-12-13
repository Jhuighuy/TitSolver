# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Find codespell.
find_program(
  CODESPELL_EXE
  NAMES "codespell")

if(CODESPELL_EXE)
  # Find list of words for codespell to ignore.
  find_file(
    CODESPELL_IGNORE_PATH
    NAMES "codespell.txt"
    PATHS "${CMAKE_SOURCE_DIR}/cmake"
    REQUIRED)
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

## Check spelling in the specified directory.
##
## Unlike other static analysis tools, I do not want to run spellcheck pear each
## target separately. There two issues: first, codespell is written in Python,
## and it's startup and execution times can be better, so it will slow down our
## build; second, I want spell checking in all indexed files, not just source
## files.
function(check_spelling TARGET DIR)
  # Should we skip analysis?
  if(SKIP_ANALYSIS)
    return()
  endif()
  # Exit early in case codespell was not found.
  if(NOT CODESPELL_EXE)
    return()
  endif()
  # Run codespell.
  add_custom_target(
    "${TARGET}_codespell"
    ## Run each time during the build process.
    ALL
    ## Check git indexed files only. The best way to do this is to ask git
    ## what files are indexed in the directory, and pass those to codespell.
    COMMAND
      git ls-files "${DIR}" | xargs
      "${CODESPELL_EXE}"
        --ignore-words="${CODESPELL_IGNORE_PATH}"
        --check-filenames
    ## Run from the specified directory, since codespell prints relative paths.
    WORKING_DIRECTORY "${DIR}"
    COMMENT "Checking spelling in ${DIR}")
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
