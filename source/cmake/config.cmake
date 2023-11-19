# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

include_guard()
include(clang)
include(gnu)
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Set a list of all configurations.
set(
  ALL_CONFIGS
  # Configuration with no optimizations enabled, suitable for debugging.
  "Debug"
  # Configuration with aggressive optimizations, suitable for production.
  "Release"
  # Same as `Debug`, but instrumented for test coverage analysis.
  "Coverage")

if(CMAKE_BUILD_TYPE)
  # Single-configuration generator is used. Check the current configuration.
  if(NOT CMAKE_BUILD_TYPE IN_LIST ALL_CONFIGS)
    list(JOIN ALL_CONFIGS ", " ALL_CONFIGS)
    message(
      FATAL_ERROR
      "Unknown build configuration ${CMAKE_BUILD_TYPE}. "
      "Choose from: ${ALL_CONFIGS}.")
  endif()
else()
  # Tell CMake about our configurations.
  set(CMAKE_CONFIGURATION_TYPES ${ALL_CONFIGS})
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Set compiler ID.
if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(CXX_COMPILER "CLANG")
else()
  string(TOUPPER ${CMAKE_CXX_COMPILER_ID} CXX_COMPILER)
endif()

# Check if compiler is known..
try_set(CXX_COMPILER_MIN_VERSION "${CXX_COMPILER}_MIN_VERSION")
if(NOT DEFINED CXX_COMPILER_MIN_VERSION)
  message(
    FATAL_ERROR
    "Compiler ${CMAKE_CXX_COMPILER_ID} is not supported.")
endif()
# ..and it's version is sufficient.
if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${CXX_COMPILER_MIN_VERSION})
  message(
    FATAL_ERROR
    "Insufficient version of compiler ${CMAKE_CXX_COMPILER_ID} is used. "
    "Minimum required version is ${CXX_COMPILER_MIN_VERSION}.")
endif()

# Setup warnings and diagnostics options.
try_set(CXX_WARNINGS "${CXX_COMPILER}_WARNINGS")

# Setup optimization options.
try_set(CXX_OPTIMIZE_OPTIONS "${CXX_COMPILER}_OPTIMIZE_OPTIONS")
foreach(CONFIG ${ALL_CONFIGS})
  string(TOUPPER ${CONFIG} CONFIG)
  try_set(
    "CXX_OPTIMIZE_OPTIONS_${CONFIG}"
    "${CXX_COMPILER}_OPTIMIZE_OPTIONS_${CONFIG}"
    ${CXX_OPTIMIZE_OPTIONS})
endforeach()

# Setup compile options (warnings + optimization).
set(CXX_COMPILE_OPTIONS ${CXX_WARNINGS} ${CXX_OPTIMIZE_OPTIONS})
foreach(CONFIG ${ALL_CONFIGS})
  string(TOUPPER ${CONFIG} CONFIG)
  set(
    "CXX_COMPILE_OPTIONS_${CONFIG}"
    ${CXX_WARNINGS}
    ${CXX_OPTIMIZE_OPTIONS_${CONFIG}})
endforeach()

# Setup link options.
try_set(CXX_LINK_OPTIONS "${CXX_COMPILER}_LINK_OPTIONS")
foreach(CONFIG ${ALL_CONFIGS})
  string(TOUPPER ${CONFIG} CONFIG)
  try_set(
    "CXX_LINK_OPTIONS_${CONFIG}"
    "${CXX_COMPILER}_LINK_OPTIONS_${CONFIG}"
    ${CXX_LINK_OPTIONS})
endforeach()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
