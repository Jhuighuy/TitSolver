# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define minimal compiler version.
set(CLANG_MIN_VERSION "19.1.5")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define warnings and diagnostics options.
set(
  CLANG_WARNINGS
  # Treat warnings as errors.
  -Werror
  # Enable most of the commonly used warning options.
  -Wall
  -Wextra
  -Wextra-semi
  -Wpedantic
  # Redeclaration of class members.
  -Wredeclared-class-member
  # Redundant declarations.
  -Wredundant-decls
  # Redundant move operations.
  -Wredundant-move
  # Redundant parentheses.
  -Wredundant-parens
  # Everything should be used.
  -Wunused-comparison
  -Wunused-const-variable
  -Wunused-exception-parameter
  -Wunused-function
  -Wunused-label
  -Wunused-lambda-capture
  -Wunused-local-typedef
  -Wunused-parameter
  -Wunused-private-field
  -Wunused-template
  -Wunused-value
  -Wunused-variable
  -Wunused-volatile-lvalue
  # No warnings for unknown pragmas.
  -Wno-unknown-pragmas
  # No warnings for unknown warning options.
  -Wno-unknown-warning-option
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
)

# When compiling with GCC, force LLVM tools to use libstdc++.
#
# It is generally hard to detect which standard library is used by the LLVM
# tools, so we have to to make an assumption here. Looks like LLVM defaults to
# libc++ on Apple platforms only, on Linux it defaults to libstdc++.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND APPLE)
  # Find path to libstdc++ include directories. It should be something like
  # `<install-path>/gcc/<version>/include/c++/<version>/` and
  # `<install-path>/gcc/<version>/include/c++/<version>/<platform>/`.
  set(
    STDCPP_INCLUDE_DIR_REGEX
    "gcc/([0-9]+(\\.[0-9]+)+(_[0-9]+)?)/include/c\\+\\+/([0-9]+)"
  )
  set(
    STDCPP_SYS_INCLUDE_DIR_REGEX
    "${STDCPP_INCLUDE_DIR_REGEX}/(.*-apple-.*)"
  )
  set(STDCPP_INCLUDE_DIR)
  set(STDCPP_SYS_INCLUDE_DIR)
  foreach(DIR ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    if(DIR MATCHES "${STDCPP_INCLUDE_DIR_REGEX}$")
      set(STDCPP_INCLUDE_DIR ${DIR})
    elseif(DIR MATCHES "${STDCPP_SYS_INCLUDE_DIR_REGEX}$")
      set(STDCPP_SYS_INCLUDE_DIR ${DIR})
    endif()
  endforeach()
  if(NOT STDCPP_INCLUDE_DIR OR NOT STDCPP_SYS_INCLUDE_DIR)
    list(JOIN CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "\n" DIRS)
    message(FATAL_ERROR "Could not find libstdc++ in:\n${DIRS}")
  endif()

  # Add libstdc++ include paths.
  list(
    APPEND
    CLANG_COMPILE_OPTIONS
    -stdlib++-isystem "${STDCPP_INCLUDE_DIR}"
    -cxx-isystem "${STDCPP_SYS_INCLUDE_DIR}"
  )
endif()

# Define common link options.
set(CLANG_LINK_OPTIONS)
if(APPLE)
  list(
    APPEND
    CLANG_LINK_OPTIONS
    # Do not warn about duplicate libraries.
    -Wl,-no_warn_duplicate_libraries
    # Link with the Homebrew LLVM libraries.
    # TODO: Here should be a check for the Homebrew LLVM installation.
    -L/opt/homebrew/opt/llvm/lib/c++
    -L/opt/homebrew/opt/llvm/lib/unwind
  )
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define common optimization options.
set(
  CLANG_OPTIMIZE_OPTIONS
  # Enable aggressive optimization levels.
  -O3
  -ffast-math
  # Enables aggressive floating-point expression contraction.
  -ffp-contract=fast
)

# Use link time optimizations?
#
# Note: This is experimental and may significantly decrease performance!
set(CLANG_USE_LTO FALSE)
if(CLANG_USE_LTO)
  message(WARNING "Link-time optimizations support is experimental!")
  list(APPEND CLANG_OPTIMIZE_OPTIONS -flto)
endif()

# Define compile options for "Release" configuration.
set(
  CLANG_COMPILE_OPTIONS_RELEASE
  # Inherit common options.
  ${CLANG_COMPILE_OPTIONS}
  # Inherit optimization options.
  ${CLANG_OPTIMIZE_OPTIONS}
)

# Define link options for "Release" configuration.
set(
  CLANG_LINK_OPTIONS_RELEASE
  # Inherit common options.
  ${CLANG_LINK_OPTIONS}
  # Inherit optimization options (needed for LTO or PGO).
  ${CLANG_OPTIMIZE_OPTIONS}
)

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
set(
  CLANG_COMPILE_OPTIONS_DEBUG
  # Inherit common options.
  ${CLANG_COMPILE_OPTIONS}
  # Inherit debugging options.
  ${CLANG_DEBUG_OPTIONS}
)

# Define link options for "Debug" configuration.
set(
  CLANG_LINK_OPTIONS_DEBUG
  # Inherit common options.
  ${CLANG_LINK_OPTIONS}
  # Inherit debugging options.
  ${CLANG_DEBUG_OPTIONS}
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
