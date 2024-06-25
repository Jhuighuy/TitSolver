# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define minimal compiler version.
# Disabled, since we do not officially support clang yet.
# set(CLANG_MIN_VERSION "18.1.5")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define warnings and diagnostics options.
set(
  CLANG_WARNINGS
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
  # `__cpp_concepts` is not updated in clang for some reason.
  -Wno-builtin-macro-redefined
  -D__cpp_concepts=202002L
)

# When compiling with GCC, force LLVM tools to use libstdc++.
#
# It is generally hard to detect which standard library is used by the LLVM
# tools, so we have to to make an assumption here. Looks like LLVM defaults to
# libc++ on Apple platforms only, on Linux it defaults to libstdc++.
if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
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

  # Clear variables.
  unset(STDCPP_INCLUDE_DIR)
  unset(STDCPP_SYS_INCLUDE_DIR)
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define compile options for "Release" configuration.
set(
  CLANG_COMPILE_OPTIONS_RELEASE
  # Inherit common options.
  ${CLANG_COMPILE_OPTIONS}
  # Enable aggressive optimization levels to maximize performance.
  -Ofast
  # Enables aggressive floating-point expression contraction.
  -ffp-contract=fast
  # Are these options necessary?
  -funroll-loops
  -finline-functions
  -fomit-frame-pointer
  -ffinite-loops
  -ffinite-math-only
  -fno-stack-protector
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define compile options for "Debug" configuration.
set(
  CLANG_COMPILE_OPTIONS_DEBUG
  # Inherit common options.
  ${CLANG_COMPILE_OPTIONS}
  # Store debug information.
  -g
  # Optimize for debugging experience.
  -Og
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
