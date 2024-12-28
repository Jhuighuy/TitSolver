# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define warnings and diagnostics options.
set(
  GNU_WARNINGS
  # Treat warnings as errors.
  -Werror
  # Enable all the commonly used warning options.
  -Wall
  -Wextra
  -Wpedantic
  # Pointer casts that increase or decrease alignment.
  -Wcast-align
  # Pointer is cast so that it removes the `const` qualifier.
  -Wcast-qual
  # Optimization pass is disabled.
  -Wdisabled-optimization
  # Duplicated or redundant conditions in code.
  -Wduplicated-cond
  # Check calls to `printf`-like functions for format string-related issues.
  -Wformat=2
  # Logical operations in code that is likely to be a mistake.
  -Wlogical-op
  # Global function is defined without a previous declaration.
  -Wmissing-declarations
  # Usage of C-style casts.
  -Wold-style-cast
  # Structures that are packed to an unusual degree.
  -Wpacked
  # Redundant declarations.
  -Wredundant-decls
  # Local variable or type declaration shadows another local variable.
  -Wshadow
  # Potential arithmetic overflows during shifts.
  -Wshift-overflow
  # No warnings for unknown warning options.
  -Wno-pragmas
  # Do not warn about potential ABI changes.
  -Wno-psabi
)

# Define common compile options.
set(
  GNU_COMPILE_OPTIONS
  # Warnings and diagnostics.
  ${GNU_WARNINGS}
  # Generate machine code for the host system's architecture.
  -march=native
  # Position independent code.
  -fPIC
)
if(APPLE)
  # Set the minimum macOS version to 15.20.
  list(APPEND GNU_COMPILE_OPTIONS -mmacosx-version-min=15.20)
endif()

# Define common link options.
set(GNU_LINK_OPTIONS ${GNU_COMPILE_OPTIONS})
if(APPLE)
  list(
    APPEND
    GNU_LINK_OPTIONS
    # Do not warn about duplicate libraries.
    -Wl,-no_warn_duplicate_libraries
  )
else()
  list(
    APPEND
    GNU_LINK_OPTIONS
    # Allow shared libraries to resolve symbols at runtime.
    -Wl,--export-dynamic
  )
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define common optimization options.
set(
  GNU_OPTIMIZE_OPTIONS
  # Enable aggressive optimization levels.
  -Ofast
  # Enable aggressive floating-point expression contraction.
  -ffp-contract=fast
  # Enable link time optimizations.
  -flto=auto
)
if(NOT APPLE)
  # Disable `-Wmaybe-uninitialized` warning, it produces many false positives.
  list(APPEND GNU_OPTIMIZE_OPTIONS -Wno-maybe-uninitialized)
endif()

# Define compile options for "Release" configuration.
set(GNU_COMPILE_OPTIONS_RELEASE ${GNU_COMPILE_OPTIONS} ${GNU_OPTIMIZE_OPTIONS})

# Define link options for "Release" configuration.
set(GNU_LINK_OPTIONS_RELEASE ${GNU_LINK_OPTIONS} ${GNU_OPTIMIZE_OPTIONS})

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
set(GNU_COMPILE_OPTIONS_DEBUG ${GNU_COMPILE_OPTIONS} ${GNU_DEBUG_OPTIONS})

# Define link options for "Debug" configuration.
set(GNU_LINK_OPTIONS_DEBUG ${GNU_LINK_OPTIONS} ${GNU_DEBUG_OPTIONS})

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Define common coverage options.
set(
  GNU_COVERAGE_OPTIONS
  # Enable code coverage instrumentation during compilation.
  --coverage
  # Update profile data atomically, since we are multi-threaded.
  -fprofile-update=atomic
  # Disable all disablable optimizations to make coverage reports more accurate.
  -O0
  -fno-default-inline
  -fno-inline
  -fno-inline-small-functions
  -fno-elide-constructors
  # Inform our code that we are compiling with gcov.
  -DTIT_HAVE_GCOV=1
)

# Define compile options for "Coverage" configuration.
set(GNU_COMPILE_OPTIONS_COVERAGE ${GNU_COMPILE_OPTIONS} ${GNU_COVERAGE_OPTIONS})

# Define the link options for "Coverage" configuration.
set(GNU_LINK_OPTIONS_COVERAGE ${GNU_LINK_OPTIONS} ${GNU_COVERAGE_OPTIONS})

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
