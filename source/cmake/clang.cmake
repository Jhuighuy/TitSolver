# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Set minimal compiler version.
# Disabled, since we do not officially support clang yet.
# set(CLANG_MIN_VERSION "17.0")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

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
  -Wno-unknown-warning-option)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
