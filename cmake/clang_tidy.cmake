# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

include_guard()
include(clang)
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Find clang-tidy (at least 16.0). Prefer version-suffixed executables.
find_program_with_version(
  CLANG_TIDY_EXE
  NAMES "clang-tidy-17" "clang-tidy-16" "clang-tidy"
  MIN_VERSION "16.0")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

## Analyze source code with clang-tidy.
function(enable_clang_tidy TARGET_OR_ALIAS)
  # Should we skip analysis?
  if(SKIP_ANALYSIS)
    return()
  endif()
  # Exit early in case sufficient clang-tidy was not found.
  if(NOT CLANG_TIDY_EXE)
    return()
  endif()
  # Get the original target name if it is an alias.
  get_target_property(TARGET ${TARGET_OR_ALIAS} ALIASED_TARGET)
  if(NOT TARGET ${TARGET})
    set(TARGET ${TARGET_OR_ALIAS})
  endif()
  # Setup common arguments for clang-tidy call.
  set(
    CLANG_TIDY_ARGS
    # No annoying output.
    --quiet
    # Enable colors during piping through chronic.
    --use-color)
  # Setup "compilation" arguments for clang-tidy call.
  ## Get generated compile options from target.
  get_generated_compile_options(${TARGET} CLANG_TIDY_COMPILE_ARGS)
  ## Append some extra options.
  list(
    APPEND
    CLANG_TIDY_COMPILE_ARGS
    # Enable the same set of warnings we would use for normal clang.
    ${CLANG_WARNINGS}
    # Enable C++23 (`c++2b` and not `c++23` for clang-16).
    -std=c++2b)
  ## Setup libstdc++ include paths when compiled with GCC.
  ## TODO: Looks like currently it only happens on macOS. This simple check
  ## should work for now.
  if(APPLE)
    list(
      APPEND
      CLANG_TIDY_COMPILE_ARGS
      # Use libstdc++ instead of libc++ as the standard library.
      -stdlib=libstdc++
      # The option above sometimes emits a warning, so we disable it.
      -Wno-unused-command-line-argument)
    ## Find path to libstdc++. Something like ".../gcc/13.2.0/include/c++/13".
    set(STDLIB_DIR)
    set(STDLIB_PLATFORM_DIR)
    set(STBLIB_DIR_REGEX "gcc/([0-9]+(\\.[0-9]+)+)/include/c\\+\\+/([0-9]+)")
    set(STBLIB_PLATFORM_DIR_REGEX "${STBLIB_DIR_REGEX}/(.*-apple-.*)")
    foreach(DIR ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
      if(DIR MATCHES "${STBLIB_DIR_REGEX}$")
        #message(STATUS "Found GCC include path: ${DIR}")
        set(STDLIB_DIR ${DIR})
      elseif(DIR MATCHES "${STBLIB_PLATFORM_DIR_REGEX}$")
        #message(STATUS "Found GCC platform include path: ${DIR}")
        set(STDLIB_PLATFORM_DIR ${DIR})
      endif()
    endforeach()
    ## Add libstdc++ include paths.
    if(STDLIB_DIR)
      list(APPEND CLANG_TIDY_COMPILE_ARGS -stdlib++-isystem "${STDLIB_DIR}")
    endif()
    if(STDLIB_PLATFORM_DIR)
      list(APPEND CLANG_TIDY_COMPILE_ARGS -cxx-isystem "${STDLIB_PLATFORM_DIR}")
    endif()
  endif()
  # Loop through the target sources and call clang-tidy.
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
    set(STAMP "${SOURCE}.tidy_stamp")
    list(APPEND ALL_STAMPS ${STAMP})
    # Execute clang-tidy and update a stamp file on success.
    # (wrapped with chronic to avoid annoying `N warnings generated` messages).
    set(SOURCE_PATH "${TARGET_SOURCE_DIR}/${SOURCE}")
    add_custom_command(
      OUTPUT ${STAMP}
      ## Execute clang-tidy.
      COMMAND
        "${CHRONIC_EXE}"
        "${CLANG_TIDY_EXE}" "${SOURCE_PATH}"
        ${CLANG_TIDY_ARGS} -- ${CLANG_TIDY_COMPILE_ARGS}
      ## Update stamp.
      COMMAND
        "${CMAKE_COMMAND}" -E touch "${STAMP}"
      ## Depend on our source file and all it's dependencies.
      DEPENDS "${SOURCE_PATH}"
      IMPLICIT_DEPENDS CXX "${SOURCE_PATH}"
      ## Add message.
      COMMENT "Analyzing ${SOURCE_PATH}"
      ## This is needed for generator expressions to work.
      COMMAND_EXPAND_LISTS
      VERBATIM)
  endforeach()
  # Create a custom target that should "build" once all checks succeed.
  add_custom_target("${TARGET}_tidy" ALL DEPENDS ${TARGET} ${ALL_STAMPS})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
