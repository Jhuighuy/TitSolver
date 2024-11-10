# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define minimal compiler version.
set(GNU_MIN_VERSION "14.2.0")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define warnings and diagnostics options.
set(
  GNU_WARNINGS
  # Treat warnings as errors.
  -Werror
  # Enable most of the commonly used warning options.
  -Wall
  -Wextra
  -Wpedantic
  # Pointer casts that increase or decrease alignment.
  -Wcast-align
  # Pointer is cast so that it removes the `const` qualifier.
  -Wcast-qual
  # Character array subscripts have type `int`.
  -Wchar-subscripts
  # Potentially ambiguous or dangling `else` clauses in code.
  -Wdangling-else
  # Optimization pass is disabled.
  -Wdisabled-optimization
  # Duplicated or redundant conditions in code.
  -Wduplicated-cond
  # Check calls to `printf`-like functions for format string-related issues.
  -Wformat=2
  # Self-initialization.
  -Winit-self
  # Logical operations in code that is likely to be a mistake.
  -Wlogical-op
  # Global function is defined without a previous declaration.
  -Wmissing-declarations
  # Usage of C-style casts.
  -Wold-style-cast
  # Structures that are packed to an unusual degree.
  -Wpacked
  # Possibly incorrect pointer arithmetic.
  -Wpointer-arith
  # Redundant declarations.
  -Wredundant-decls
  # Incorrect use of the `restrict` keyword.
  -Wrestrict
  # Local variable or type declaration shadows another local variable.
  -Wshadow
  # Left-shifting negative values.
  -Wshift-negative-value
  # Potential arithmetic overflows during shifts.
  -Wshift-overflow
  # `switch` statement does not have a `default` case.
  -Wswitch-enum
  # Use of uninitialized variables.
  -Wuninitialized
  # String constants are passed to non-`const char*` parameters.
  -Wwrite-strings
  # Violations of pointer safety annotations (disabled).
  -Wno-psabi
)

# Define common compile options.
set(
  GNU_COMPILE_OPTIONS
  # Warnings and diagnostics.
  ${GNU_WARNINGS}
  # Generate machine code for the host system's architecture.
  -march=native
)

# Define common link options.
set(GNU_LINK_OPTIONS)
if(APPLE)
  # Do not warn about duplicate libraries.
  list(APPEND GNU_LINK_OPTIONS -Wl,-no_warn_duplicate_libraries)
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define common optimization options.
set(
  GNU_OPTIMIZE_OPTIONS
  # Enable aggressive optimization levels.
  -Ofast
  # Enables aggressive floating-point expression contraction.
  -ffp-contract=fast
)

# Use link time optimizations?
#
# Note: This is experimental and may significantly decrease performance!
set(GNU_USE_LTO FALSE)
if(GNU_USE_LTO)
  message(WARNING "Link-time optimizations support is experimental!")
  list(APPEND GNU_OPTIMIZE_OPTIONS -flto=auto)
endif()

# Use profile-guided optimizations?
#
# Note: This is experimental and may significantly decrease performance!
set(GNU_USE_PGO FALSE)
if(GNU_USE_PGO)
  message(WARNING "Profile-guided optimizations support is experimental!")

  # Setup the directory for profile data.
  set(GNU_PGO_DIR "${CMAKE_BINARY_DIR}/gnu_pgo")
  file(MAKE_DIRECTORY "${GNU_PGO_DIR}")

  # Check if the directory is empty.
  file(GLOB RESULT "${GNU_PGO_DIR}/*")
  list(LENGTH RESULT RESULT)
  if(RESULT EQUAL 0)
    # If the directory is empty, we need to generate the profile.
    message(WARNING "PGO: Profile data will be generated.")
    list(
      APPEND
      GNU_OPTIMIZE_OPTIONS
      # Generate profile data.
      "-fprofile-dir=${GNU_PGO_DIR}"
      "-fprofile-generate=${GNU_PGO_DIR}"
      # Update profile data atomically, since we are multi-threaded.
      -fprofile-update=atomic
    )
  else()
    # If the directory is not empty, we can use the profile.
    message(WARNING "PGO: Profile data will be used.")
    list(
      APPEND
      GNU_OPTIMIZE_OPTIONS
      # Use profile data.
      "-fprofile-dir=${GNU_PGO_DIR}"
      "-fprofile-use=${GNU_PGO_DIR}"
      # Profiling data may be slightly inaccurate or incomplete.
      -fprofile-correction
      -Wno-missing-profile
    )
  endif()
endif()

# Define compile options for "Release" configuration.
set(
  GNU_COMPILE_OPTIONS_RELEASE
  # Inherit common options.
  ${GNU_COMPILE_OPTIONS}
  # Inherit optimization options.
  ${GNU_OPTIMIZE_OPTIONS}
)

# Define link options for "Release" configuration.
set(
  GNU_LINK_OPTIONS_RELEASE
  # Inherit common options.
  ${GNU_LINK_OPTIONS}
  # Inherit optimization options.
  ${GNU_OPTIMIZE_OPTIONS}
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define common debugging options.
set(
  GNU_DEBUG_OPTIONS
  # Store debug information.
  -g
  # Optimize for debugging experience.
  -Og
)

# Define compile options for "Debug" configuration.
set(
  GNU_COMPILE_OPTIONS_DEBUG
  # Inherit common options.
  ${GNU_COMPILE_OPTIONS}
  # Inherit debugging options.
  ${GNU_DEBUG_OPTIONS}
)

# Define link options for "Debug" configuration.
set(
  GNU_LINK_OPTIONS_DEBUG
  # Inherit common options.
  ${GNU_LINK_OPTIONS}
  # Inherit debugging options.
  ${GNU_DEBUG_OPTIONS}
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define common coverage options.
set(
  GNU_COVERAGE_OPTIONS
  # Enable code coverage instrumentation during compilation.
  --coverage
  # Update profile data atomically, since we are multi-threaded.
  -fprofile-update=atomic
)

# Define compile options for "Coverage" configuration.
set(
  GNU_COMPILE_OPTIONS_COVERAGE
  # Inherit common options.
  ${GNU_COMPILE_OPTIONS}
  # Inherit coverage options.
  ${GNU_COVERAGE_OPTIONS}
  # Disable optimizations.
  -O0
  # Pass a definition that we are compiling with gcov.
  -DTIT_HAVE_GCOV=1
  # Avoid inlining and elisions, which can make coverage reports less accurate.
  -fno-default-inline
  -fno-inline
  -fno-inline-small-functions
  -fno-elide-constructors
)

# Define the link options for "Coverage" configuration.
set(
  GNU_LINK_OPTIONS_COVERAGE
  # Inherit common options.
  ${GNU_LINK_OPTIONS}
  # Inherit coverage options.
  ${GNU_COVERAGE_OPTIONS}
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
