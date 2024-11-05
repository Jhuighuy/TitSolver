# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(clang)
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find clang-tidy (at least 18.1.3). Prefer version-suffixed executables.
find_program(CLANG_TIDY_EXE NAMES "clang-tidy-18" "clang-tidy")
if(NOT CLANG_TIDY_EXE)
  message(WARNING "clang-tidy was not found!")
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Analyze source code with clang-tidy.
#
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
    --use-color
  )

  # Disable a few checks when compiling with Clang.
  # TODO: All the checks must be enabled at some point!
  if(CXX_COMPILER STREQUAL "CLANG")
    set(
      LIBCPP_DISABLED_CHECKS
      # False positives with standard headers, like <format> and <expected>.
      # Looks like this issue is fixed in LLVM 19.
      -misc-include-cleaner
      # Suppress error messages from Doctest.
      -modernize-type-traits
    )
    list(JOIN LIBCPP_DISABLED_CHECKS "," LIBCPP_DISABLED_CHECKS)
    list(APPEND CLANG_TIDY_ARGS "--checks=${LIBCPP_DISABLED_CHECKS}")
  endif()

  # Setup "compilation" arguments for clang-tidy call.
  get_generated_compile_options(${TARGET} CLANG_TIDY_COMPILE_ARGS)
  list(
    APPEND
    CLANG_TIDY_COMPILE_ARGS
    # Inherit compile options we would use for compilation.
    ${CLANG_COMPILE_OPTIONS}
    # Enable C++23, as it is not configured by the compile options.
    -std=c++23
  )

  # Get the source directory and sources of the target.
  set(ALL_STAMPS)
  get_target_property(TARGET_SOURCE_DIR ${TARGET} SOURCE_DIR)
  get_target_property(TARGET_SOURCES ${TARGET} SOURCES)
  if(NOT TARGET_SOURCES)
    message(WARNING "clang-tidy: no sources found for target ${TARGET}!")
    return()
  endif()

  # Loop through the target sources and call clang-tidy.
  foreach(SOURCE ${TARGET_SOURCES})
    # Skip non-C/C++ files.
    is_cpp_file("${SOURCE}" SOURCE_IS_CPP)
    if(NOT SOURCE_IS_CPP)
      message(WARNING "clang-tidy: a skipping non C/C++ file: '${SOURCE}'.")
      continue()
    endif()

    # Create a stamp.
    string(REPLACE "/" "_" STAMP "${SOURCE}.tidy_stamp")
    list(APPEND ALL_STAMPS "${STAMP}")

    # Execute clang-tidy and update a stamp file on success.
    # (wrapped with chronic to avoid annoying `N warnings generated` messages).
    set(SOURCE_PATH "${TARGET_SOURCE_DIR}/${SOURCE}")
    add_custom_command(
      COMMENT "Analyzing ${SOURCE_PATH}"
      OUTPUT "${STAMP}"
      COMMAND
        "${CHRONIC_EXE}"
        "${CLANG_TIDY_EXE}" "${SOURCE_PATH}"
        ${CLANG_TIDY_ARGS} -- ${CLANG_TIDY_COMPILE_ARGS}
      COMMAND
        "${CMAKE_COMMAND}" -E touch "${STAMP}"
      DEPENDS "${SOURCE_PATH}"
      IMPLICIT_DEPENDS CXX "${SOURCE_PATH}"
      COMMAND_EXPAND_LISTS # Needed for generator expressions to work.
      VERBATIM
    )
  endforeach()

  # Create a custom target that should "build" once all checks succeed.
  add_custom_target("${TARGET}_tidy" ALL DEPENDS ${TARGET} ${ALL_STAMPS})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
