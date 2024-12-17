# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(clang)
include(gnu)
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Set a list of all configurations.
set(
  ALL_CONFIGS
  # Configuration with no optimizations enabled, suitable for debugging.
  "Debug"
  # Configuration with aggressive optimizations, suitable for production.
  "Release"
  # Same as `Debug`, but instrumented for test coverage analysis.
  "Coverage"
)

# Check the current configuration.
if(NOT CMAKE_BUILD_TYPE IN_LIST ALL_CONFIGS)
  list(JOIN ALL_CONFIGS ", " ALL_CONFIGS)
  message(
    FATAL_ERROR
    "Build configuration must be one of the following: ${ALL_CONFIGS}."
  )
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Setup the compiler ID.
string(TOUPPER ${CMAKE_CXX_COMPILER_ID} CXX_COMPILER)

# Setup compile options.
set(CXX_COMPILE_OPTIONS "${${CXX_COMPILER}_COMPILE_OPTIONS}")
foreach(CONFIG ${ALL_CONFIGS})
  string(TOUPPER ${CONFIG} CONFIG)
  set(
    "CXX_COMPILE_OPTIONS_${CONFIG}"
    "${${CXX_COMPILER}_COMPILE_OPTIONS_${CONFIG}}"
  )
endforeach()

# Setup link options.
set(CXX_LINK_OPTIONS "${CXX_COMPILER}_LINK_OPTIONS")
foreach(CONFIG ${ALL_CONFIGS})
  string(TOUPPER ${CONFIG} CONFIG)
  set(
    "CXX_LINK_OPTIONS_${CONFIG}"
    "${${CXX_COMPILER}_LINK_OPTIONS_${CONFIG}}"
  )
endforeach()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
