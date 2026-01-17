# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
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
)

# Define common compile options.
set(
  CLANG_COMPILE_OPTIONS
  # Use C++26 standard.
  -std=c++26
  # Warnings and diagnostics.
  ${CLANG_WARNINGS}
  # Generate machine code for the host system's architecture.
  -march=native
  # Position independent code.
  -fPIC
  # Do not export symbols.
  -fvisibility=hidden
  # As of LLVM 21.1.8, builtin for `std::forward_like` is bogus.
  -fno-builtin-std-forward_like
  # As of LLVM 21.1.8, C++26's `std::format` is not working properly.
  # https://github.com/llvm/llvm-project/issues/151371
  -D__cpp_lib_format=202304L
  -D__glibcxx_format=202304L
)

# Define common link options.
set(CLANG_LINK_OPTIONS ${CLANG_COMPILE_OPTIONS})
if(APPLE)
  # Do not warn about duplicate libraries.
  list(APPEND CLANG_LINK_OPTIONS -Wl,-no_warn_duplicate_libraries)
endif()

# When the actual compiler is configured to use libstdc++, I want the LLVM tools
# invoked externally (e.g. clang-tidy, clangd) to use exactly the same libstdc++
# as well. Paths to the libstdc++ headers are usually found in the implicit
# include directories, they look something like this:
#
# `<install-path>/gcc/[<version>/]include/c++/<version>/` and
# `<install-path>/gcc/[<version>/]include/c++/<version>/<platform>/`.
function(_make_libstdcpp_options RESULT_VAR)
  set(FOUND_DIR)
  set(DIR_REGEX "gcc/([1-9]+(\\.[0-9]+)*(_[0-9]+)?/)?include/c\\+\\+/[1-9]+")
  foreach(DIR ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    if(DIR MATCHES "${DIR_REGEX}$")
      set(FOUND_DIR "${DIR}")
      break() # I hope the first match is the right one.
    endif()
  endforeach()
  if(NOT FOUND_DIR)
    return()
  endif()

  set(FOUND_PLATFORM_DIR)
  set(PLATFORM_DIR_REGEX "${DIR_REGEX}/[^/]*-(apple|linux)-[^/]*")
  foreach(DIR ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    if(DIR STRGREATER "${FOUND_DIR}" AND DIR MATCHES "${PLATFORM_DIR_REGEX}$")
      set(FOUND_PLATFORM_DIR "${DIR}")
      break()
    endif()
  endforeach()
  if(NOT FOUND_PLATFORM_DIR)
    return()
  endif()

  set(
    ${RESULT_VAR}
    -stdlib++-isystem "${FOUND_DIR}"
    -cxx-isystem "${FOUND_PLATFORM_DIR}"
    PARENT_SCOPE
  )
endfunction()
_make_libstdcpp_options(CLANG_STDLIB_OPTIONS)
unset(_make_libstdcpp_options)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define common optimization options.
set(
  CLANG_OPTIMIZE_OPTIONS
  # Enable aggressive optimization levels.
  -O3
  # Enable aggressive floating-point expression contraction.
  -ffast-math
  -ffp-contract=fast
  # Link time optimizations are disabled: we experience crashes in tests.
  # -flto=auto
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
find_program(CLANG_TIDY_EXE NAMES "clang-tidy-21" "clang-tidy")

# Define clang-tidy options.
set(
  CLANG_TIDY_OPTIONS
  # No annoying output.
  --quiet
  # Enable colors during piping through chronic.
  --use-color
)

#
# Analyze source files with clang-tidy.
#
function(enable_clang_tidy TARGET)
  # Exit early in case sufficient clang-tidy was not found.
  if(NOT CLANG_TIDY_EXE)
    return()
  endif()

  # Setup "compilation" arguments for clang-tidy call.
  get_generated_compile_options(${TARGET} TARGET_COMPILE_OPTIONS)
  list(
    APPEND
    TARGET_COMPILE_OPTIONS
    # Inherit compile options we would use for compilation.
    ${CLANG_COMPILE_OPTIONS}
    ${CLANG_STDLIB_OPTIONS}
  )

  # Get the binary, source directory and sources of the target.
  get_target_property(TARGET_SOURCES ${TARGET} SOURCES)
  get_target_property(TARGET_SOURCE_DIR ${TARGET} SOURCE_DIR)
  get_target_property(TARGET_BINARY_DIR ${TARGET} BINARY_DIR)

  # Loop through the target sources and call clang-tidy.
  set(ALL_STAMPS)
  foreach(SOURCE ${TARGET_SOURCES})
    # We'll need a stamp file to track the analysis.
    string(REPLACE "/" "_" STAMP "${SOURCE}.stamp")
    set(STAMP "${TARGET_BINARY_DIR}/${STAMP}")
    list(APPEND ALL_STAMPS "${STAMP}")

    # Locate the source file.
    if(NOT EXISTS "${SOURCE}")
      if(EXISTS "${TARGET_SOURCE_DIR}/${SOURCE}")
        set(SOURCE "${TARGET_SOURCE_DIR}/${SOURCE}")
      elseif(EXISTS "${TARGET_BINARY_DIR}/${SOURCE}")
        set(SOURCE "${TARGET_BINARY_DIR}/${SOURCE}")
      else()
        message(WARNING "Source file '${SOURCE}' cannot be found.")
        continue()
      endif()
    endif()

    # Find all the '.clang-tidy' files in the source tree.
    set(CLANG_TIDY_CONFIG_FILES)
    get_filename_component(CURRENT_DIR "${SOURCE}" DIRECTORY)
    while(true)
      if(EXISTS "${CURRENT_DIR}/.clang-tidy")
        list(APPEND CLANG_TIDY_CONFIG_FILES "${CURRENT_DIR}/.clang-tidy")
      endif()
      if(CURRENT_DIR STREQUAL "${CMAKE_SOURCE_DIR}")
        break() # Reached the root of the source tree.
      endif()
      get_filename_component(CURRENT_DIR "${CURRENT_DIR}" DIRECTORY)
    endwhile()

    # Run clang-tidy and update a stamp file on success.
    cmake_path(
      RELATIVE_PATH SOURCE
      BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_VARIABLE RELATIVE_SOURCE_PATH
    )
    add_custom_command(
      COMMENT "Analyzing ${RELATIVE_SOURCE_PATH}"
      OUTPUT "${STAMP}"
      COMMAND
        "${CHRONIC_EXE}"
          "${CLANG_TIDY_EXE}"
            "${SOURCE}" ${CLANG_TIDY_OPTIONS} -- ${TARGET_COMPILE_OPTIONS}
      COMMAND
        "${CMAKE_COMMAND}" -E touch "${STAMP}"
      DEPENDS "${SOURCE}" ${CLANG_TIDY_CONFIG_FILES}
      IMPLICIT_DEPENDS CXX "${SOURCE}"
      COMMAND_EXPAND_LISTS # Needed for generator expressions to work.
      VERBATIM
    )
  endforeach()

  # Create a custom target that should "build" once all checks succeed.
  add_custom_target("${TARGET}_tidy" ALL DEPENDS ${ALL_STAMPS})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Write the `compile_flags.txt` for the clangd language server.
#
function(write_compile_flags TARGET)
  # Setup "compilation" arguments for a hypothetical "clang" call.
  get_generated_compile_options(${TARGET} TARGET_COMPILE_OPTIONS)
  list(
    APPEND
    TARGET_COMPILE_OPTIONS
    # Inherit compile options we would use for compilation.
    ${CLANG_COMPILE_OPTIONS}
    ${CLANG_STDLIB_OPTIONS}
  )

  # Remove `-Werror` from the compile flags, as it crashes clangd sometimes.
  list(REMOVE_ITEM TARGET_COMPILE_OPTIONS "-Werror")

  # Write the compile flags to a file, each on a new line.
  add_custom_target(
    "${TARGET}_compile_flags"
    ALL # Execute on every build.
    COMMAND
      "${CMAKE_COMMAND}" -E echo ${TARGET_COMPILE_OPTIONS} |
      "${XARGS_EXE}" -n 1 > "${CMAKE_SOURCE_DIR}/compile_flags.txt"
    COMMENT "Writing compile_flags.txt"
    COMMAND_EXPAND_LISTS # Needed for generator expressions to work.
    VERBATIM
  )
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
