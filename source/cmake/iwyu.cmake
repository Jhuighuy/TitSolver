# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

include_guard()
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Find include-what-you-use (at least 0.21 based on clang 17.0).
find_program_with_version(
  IWYU_EXE
  NAMES "include-what-you-use"
  MIN_VERSION "0.21")

if(IWYU_EXE)
  # Find include-what-you-use mapping file.
  find_file(
    IWYU_MAPPING_PATH
    NAMES "iwyu.imp"
    PATHS "${ROOT_SOURCE_DIR}/cmake"
    REQUIRED)
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

## Check `#includes` in the target with include-what-you-use.
function(check_includes TARGET_OR_ALIAS)
  # Should we skip analysis?
  if(SKIP_ANALYSIS)
    return()
  endif()
  # Exit early in case sufficient IWYU was not found.
  if(NOT IWYU_EXE)
    return()
  endif()
  # Get the original target name if it is an alias.
  get_target_property(TARGET ${TARGET_OR_ALIAS} ALIASED_TARGET)
  if(NOT TARGET ${TARGET})
    set(TARGET ${TARGET_OR_ALIAS})
  endif()
  # Setup common arguments for IWYU call.
  set(
    IWYU_ARGS
    # Disable forward declarations.
    -Xiwyu --no_fwd_decls
    # Treat warnings as errors.
    -Xiwyu --error
    # Provide our mapping file.
    -Xiwyu --mapping_file=${IWYU_MAPPING_PATH})
  # Setup "compilation" arguments for IWYU call.
  ## Get generated compile options from target.
  get_generated_compile_options(${TARGET} IWYU_COMPILE_ARGS)
  ## Append some extra options (no need to enable warning here).
  list(
    APPEND
    IWYU_COMPILE_ARGS
    # Enable C++23 (`c++2b` and not `c++23` for clang-16).
    -std=c++2b
    # Tell our code that it is parsed with IWYU to workaround some bugs.
    -DTIT_IWYU=1)
  # Loop through the target sources and call IWYU.
  set(ALL_STAMPS)
  get_target_property(TARGET_SOURCE_DIR ${TARGET} SOURCE_DIR)
  get_target_property(TARGET_SOURCES ${TARGET} SOURCES)
  foreach(SOURCE ${TARGET_SOURCES})
    # Skip non-C/C++ files.
    is_cxx_source(${SOURCE} SOURCE_IS_CXX)
    if(NOT SOURCE_IS_CXX)
       continue()
    endif()
    # Create stamp.
    set(STAMP "${SOURCE}.iwuy_stamp")
    list(APPEND ALL_STAMPS ${STAMP})
    # Execute include-what-you-use and update a stamp file on success
    # (wrapped with chronic to avoid annoying messages of success).
    set(SOURCE_PATH "${TARGET_SOURCE_DIR}/${SOURCE}")
    add_custom_command(
      OUTPUT ${STAMP}
      ## Execute IWYU.
      COMMAND
        "${CHRONIC_EXE}"
        "${IWYU_EXE}" "${SOURCE_PATH}" ${IWYU_ARGS} ${IWYU_COMPILE_ARGS}
      ## Update stamp.
      COMMAND
        "${CMAKE_COMMAND}" -E touch "${STAMP}"
      MAIN_DEPENDENCY "${SOURCE_PATH}"
      ## Also check all the dependant files.
      IMPLICIT_DEPENDS CXX "${SOURCE_PATH}"
      COMMENT "Checking includes in ${SOURCE_PATH}"
      ## This is needed for generator expressions to work.
      COMMAND_EXPAND_LISTS
      VERBATIM)
    endforeach()
  # Create a custom target that should "build" once all checks succeed.
  add_custom_target("${TARGET}_iwyu" ALL DEPENDS ${TARGET} ${ALL_STAMPS})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
