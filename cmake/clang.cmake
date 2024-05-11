# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Set minimal compiler version.
# Disabled, since we do not officially support clang yet.
# set(CLANG_MIN_VERSION "18.1.5")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Set warnings and diagnostics options.
set(
  CLANG_WARNINGS
  # Enable most of the commonly used warning options.
  -Wall
  # Enable extra warning options not included in `-Wall`.
  -Wextra
  # Warns about unnecessary extra semicolons in the code.
  -Wextra-semi
  # Issue all the warnings demanded by strict ISO C and ISO C++ standards.
  -Wpedantic
  # Warn about redeclaration of class members.
  -Wredeclared-class-member
  # Warn about redundant declarations.
  -Wredundant-decls
  # Warn about redundant move operations.
  -Wredundant-move
  # Warn about redundant parentheses.
  -Wredundant-parens
  # Warn about unused comparisons.
  -Wunused-comparison
  # Warn about unused `const` variables.
  -Wunused-const-variable
  # Warn about unused exception parameters.
  -Wunused-exception-parameter
  # Warn about unused functions.
  -Wunused-function
  # Warn about unused labels.
  -Wunused-label
  # Warn about unused lambda captures.
  -Wunused-lambda-capture
  # Warn about unused local typedefs.
  -Wunused-local-typedef
  # Warn about unused function parameters.
  -Wunused-parameter
  # Warn about unused private class fields.
  -Wunused-private-field
  # Warn about unused templates.
  -Wunused-template
  # Warn about unused values.
  -Wunused-value
  # Warn about unused variables.
  -Wunused-variable
  # Warn about unused volatile lvalues.
  -Wunused-volatile-lvalue
  # Disable warnings for unknown pragmas.
  -Wno-unknown-pragmas
  # Disable warnings for unknown warning options.
  -Wno-unknown-warning-option
  # `__cpp_concepts` is not updated in clang for some reason.
  -Wno-builtin-macro-redefined
  -D__cpp_concepts=202002L)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Set common compile options.
set(
  CLANG_COMPILE_OPTIONS
  # Always store debug information.
  -g
  # Generate machine code for the host system's architecture.
  -march=native)

# Set compile options for "Release" configuration.
set(
  CLANG_COMPILE_OPTIONS_RELEASE
  # Inherit common options.
  ${CLANG_COMPILE_OPTIONS}
  # Enable aggressive optimization levels to maximize performance.
  -Ofast
  # Enables aggressive floating-point expression contraction.
  -ffp-contract=fast
  -finline-functions
  -funroll-loops
  -fomit-frame-pointer
  -ffinite-loops
  -ffinite-math-only
  -fno-stack-protector)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

##
## Add compilation options that switch clang from libc++ to libstdcpp++ when
## compiling with GCC.
##
## * std::views::chunk is missing.
## * std::views::enumerate is missing.
## * Some problems with umbrella headers in include-cleaner.
##
function(clang_force_use_libstdcpp OPTIONS_VAR)
  list(
    APPEND
    ${OPTIONS_VAR}
    # Use libstdc++ instead of libc++ as the standard library.
    -stdlib=libstdc++
    # The option above sometimes emits a warning, so we disable it.
    -Wno-unused-command-line-argument)
  ## Find path to libstdc++ include directories. It should be something like
  ## ".../gcc/13.2.0/include/c++/13" & ".../gcc/13.2.0/include/c++/13/platform".
  set(LIBSTDCPP_INCLUDE_DIR)
  set(LIBSTDCPP_PLATFORM_INCLUDE_DIR)
  set(
    LIBSTDCPP_INCLUDE_DIR_REGEX
    "gcc/([0-9]+(\\.[0-9]+)+)/include/c\\+\\+/([0-9]+)")
  set(
    LIBSTDCPP_PLATFORM_INCLUDE_DIR_REGEX
    "${LIBSTDCPP_INCLUDE_DIR_REGEX}/(.*-apple-.*)")
  foreach(DIR ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    if(DIR MATCHES "${LIBSTDCPP_INCLUDE_DIR_REGEX}$")
      set(LIBSTDCPP_INCLUDE_DIR ${DIR})
    elseif(DIR MATCHES "${LIBSTDCPP_PLATFORM_INCLUDE_DIR_REGEX}$")
      set(LIBSTDCPP_PLATFORM_INCLUDE_DIR ${DIR})
    endif()
  endforeach()
  ## Add found libstdc++ include paths.
  if(LIBSTDCPP_INCLUDE_DIR)
    list(
      APPEND
      ${OPTIONS_VAR}
      -stdlib++-isystem "${LIBSTDCPP_INCLUDE_DIR}")
  endif()
  if(LIBSTDCPP_PLATFORM_INCLUDE_DIR)
    list(
      APPEND
      ${OPTIONS_VAR}
      -cxx-isystem "${LIBSTDCPP_PLATFORM_INCLUDE_DIR}")
  endif()
  ## Propagate the options to the parent scope.
  set(${OPTIONS_VAR} ${${OPTIONS_VAR}} PARENT_SCOPE)
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
