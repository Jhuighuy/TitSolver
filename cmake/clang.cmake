# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define warnings and diagnostics options.
set(
  CLANG_WARNINGS
  # Treat warnings as errors.
  -Werror
  # Enable all the commonly used warning options.
  -Wall
  -Wextra
  -Wpedantic
  -Wgnu
  # No warnings for unknown warning options.
  -Wno-unknown-warning-option
  -Wno-gnu-zero-variadic-macro-arguments
  -Wno-deprecated-literal-operator
)

# Define common compile options.
set(
  CLANG_COMPILE_OPTIONS
  # Warnings and diagnostics.
  ${CLANG_WARNINGS}
  # Generate machine code for the host system's architecture.
  -march=native
  # Position independent code.
  -fPIC
  # Do not export symbols.
  -fvisibility=hidden
  # Bug in LLVM.
  -fno-builtin-std-forward_like
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define common optimization options.
set(
  CLANG_OPTIMIZE_OPTIONS
  # Enable aggressive optimization levels.
  -O3
  -ffast-math
  # Enable aggressive floating-point expression contraction.
  -ffp-contract=fast
)

# Define compile options for "Release" configuration.
set(
  CLANG_COMPILE_OPTIONS_RELEASE
  ${CLANG_COMPILE_OPTIONS}
  ${CLANG_OPTIMIZE_OPTIONS}
)

# Define link options for "Release" configuration.
set(CLANG_LINK_OPTIONS_RELEASE ${CLANG_LINK_OPTIONS} ${CLANG_OPTIMIZE_OPTIONS})

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define common debugging options.
set(
  CLANG_DEBUG_OPTIONS
  # Store debug information.
  -g
  # Optimize for debugging experience.
  -Og
)

# Define compile options for "Debug" configuration.
set(CLANG_COMPILE_OPTIONS_DEBUG ${CLANG_COMPILE_OPTIONS} ${CLANG_DEBUG_OPTIONS})

# Define link options for "Debug" configuration.
set(CLANG_LINK_OPTIONS_DEBUG ${CLANG_LINK_OPTIONS} ${CLANG_DEBUG_OPTIONS})

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find clang-tidy. Prefer version-suffixed executables.
find_program(CLANG_TIDY_EXE NAMES "clang-tidy-20" "clang-tidy")
if(NOT CLANG_TIDY_EXE)
  message(WARNING "clang-tidy was not found!")
endif()

#
# Analyze source files of a target with clang-tidy.
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

  # TODO: As of LLVM 20.1.1, clang-tidy crashes somewhere in libstdc++ headers.
  #       Crash happens somewhere in `clang-analyzer` checks, but I cannot
  #       figure out which one exactly.
  if(NOT APPLE)
    list(APPEND CLANG_TIDY_ARGS "--checks=-clang-analyzer*")
  endif()

  # Setup "compilation" arguments for clang-tidy call.
  get_generated_compile_options(${TARGET} CLANG_TIDY_COMPILE_OPTIONS)
  list(
    APPEND
    CLANG_TIDY_COMPILE_OPTIONS
    # Inherit compile options we would use for compilation.
    ${CLANG_COMPILE_OPTIONS}
    # Enable C++23, as it is not configured by the compile options.
    -std=c++23
  )

  # Get the binary, source directory and sources of the target.
  get_target_property(TARGET_SOURCES ${TARGET} SOURCES)
  if(NOT TARGET_SOURCES)
    message(WARNING "clang-tidy: no sources found for target ${TARGET}!")
    return()
  endif()
  get_target_property(TARGET_SOURCE_DIR ${TARGET} SOURCE_DIR)
  get_target_property(TARGET_BINARY_DIR ${TARGET} BINARY_DIR)

  # Loop through the target sources and call clang-tidy.
  set(ALL_STAMPS)
  foreach(SOURCE ${TARGET_SOURCES})
    # Skip non-C/C++ files.
    is_cpp_file("${SOURCE}" SOURCE_IS_CPP)
    if(NOT SOURCE_IS_CPP)
      message(WARNING "clang-tidy: skipping a non C/C++ file: '${SOURCE}'.")
      continue()
    endif()

    # Execute clang-tidy and update a stamp file on success.
    # (wrapped with chronic to avoid annoying `N warnings generated` messages).
    string(REPLACE "/" "_" STAMP "${SOURCE}.tidy_stamp")
    set(STAMP "${TARGET_BINARY_DIR}/${STAMP}")
    list(APPEND ALL_STAMPS "${STAMP}")
    set(SOURCE_PATH "${TARGET_SOURCE_DIR}/${SOURCE}")
    cmake_path(
      RELATIVE_PATH SOURCE_PATH
      BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_VARIABLE RELATIVE_SOURCE_PATH
    )
    add_custom_command(
      COMMENT "Analyzing ${RELATIVE_SOURCE_PATH}"
      OUTPUT "${STAMP}"
      COMMAND
        "${CHRONIC_EXE}"
          "${CLANG_TIDY_EXE}"
            "${SOURCE_PATH}"
              ${CLANG_TIDY_ARGS} -- ${CLANG_TIDY_COMPILE_OPTIONS}
      COMMAND
        "${CMAKE_COMMAND}" -E touch "${STAMP}"
      DEPENDS "${SOURCE_PATH}" "${CMAKE_SOURCE_DIR}/.clang-tidy"
      IMPLICIT_DEPENDS CXX "${SOURCE_PATH}"
      COMMAND_EXPAND_LISTS # Needed for generator expressions to work.
      VERBATIM
    )
  endforeach()

  # Create a custom target that should "build" once all checks succeed.
  add_custom_target("${TARGET}_tidy" ALL DEPENDS ${TARGET} ${ALL_STAMPS})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Write the `compile_flags.txt` for the clangd language server.
#
function(write_compile_flags SAMPLE_TARGET)
  # Setup "compilation" arguments for hypothetical "clang" call.
  set(CLANG_COMPILE_ARGS)
  get_generated_compile_options(${SAMPLE_TARGET} CLANG_COMPILE_ARGS)
  list(
    APPEND
    CLANG_COMPILE_ARGS
    # Inherit compile options we would use for compilation.
    ${CLANG_COMPILE_OPTIONS}
    # Enable C++23, as it is not configured by the compile options.
    -std=c++23
  )

  # Remove `-Werror` from the compile flags, as it breaks clangd.
  list(REMOVE_ITEM CLANG_COMPILE_ARGS "-Werror")

  # Write the compile flags to a file, each on a new line.
  add_custom_target(
    "${CMAKE_PROJECT_NAME}_clangd_compile_flags"
    ALL # Execute on every build.
    COMMAND
      "${CMAKE_COMMAND}" -E echo ${CLANG_COMPILE_ARGS} |
      xargs -n 1 > "${CMAKE_SOURCE_DIR}/compile_flags.txt"
    COMMENT "Writing compile_flags.txt"
    COMMAND_EXPAND_LISTS # Needed for generator expressions to work.
    VERBATIM
  )
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
