# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find the npm executable.
find_program(NPM_EXE NAMES "npm")
if(NOT NPM_EXE)
  message(WARNING "npm was not found, frontend will not be built!")
  return()
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find all the source files.
set(SOURCES "")
foreach(EXT ".js" ".jsx" ".ts" ".tsx" ".css" ".json" ".html")
  # Configuration files in the root directory.
  file(GLOB TMP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*${EXT}")
  list(APPEND SOURCES ${TMP_SOURCES})
  # Source files, excluding tests.
  file(GLOB_RECURSE TMP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*${EXT}")
  list(FILTER TMP_SOURCES EXCLUDE REGEX "\\.test\\..*")
  list(APPEND SOURCES ${TMP_SOURCES})
endforeach()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

##
## Build the frontend application.
##
function(add_tit_front)
  # Create a stamp.
  set(STAMP "${CMAKE_CURRENT_BINARY_DIR}/front.stamp")
  # Execute npm commands.
  add_custom_command(
    OUTPUT "${STAMP}"
    ## Run from the current source directory.
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    ## Preinstall dependencies.
    COMMAND "${CHRONIC_EXE}" "${NPM_EXE}" install
    ## Run the build script.
    COMMAND "${CHRONIC_EXE}" "${NPM_EXE}" run package
    ## Update stamp.
    COMMAND "${CMAKE_COMMAND}" -E touch "${STAMP}"
    ## Depend on the frontend files.
    DEPENDS ${SOURCES}
    ## Add message.
    COMMENT "Building Electron app ${CMAKE_CURRENT_SOURCE_DIR}")
  # Add target that would be built once frontend files are updated.
  add_custom_target(tit_front ALL DEPENDS "${STAMP}")
endfunction()

add_tit_front()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

##
## Lint the frontend application code.
##
function(lint_tit_front)
  # Should we skip analysis?
  if(SKIP_ANALYSIS)
    return()
  endif()
  # Create a stamp.
  set(STAMP "${CMAKE_CURRENT_BINARY_DIR}/front-lint.stamp")
  # Execute npm commands.
  add_custom_command(
    OUTPUT "${STAMP}"
    ## Run from the current source directory.
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    ## Run the linter.
    COMMAND "${CHRONIC_EXE}" "${NPM_EXE}" run lint
    ## Update stamp.
    COMMAND "${CMAKE_COMMAND}" -E touch "${STAMP}"
    ## Depend on the frontend files.
    DEPENDS ${SOURCES}
    ## Add message.
    COMMENT "Linting Electron app ${CMAKE_CURRENT_SOURCE_DIR}")
  # Add target that would be built once frontend files are updated.
  add_custom_target(tit_front_lint ALL DEPENDS "${STAMP}")
  # Make sure that the linter is run after the build.
  add_dependencies(tit_front_lint tit_front)
endfunction()

lint_tit_front()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
