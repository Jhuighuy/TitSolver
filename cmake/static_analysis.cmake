# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

include_guard()
include(clang_tidy)
include(codespell)
include(iwyu)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

## Enable static analysis.
function(enable_static_analysis TARGET_OR_ALIAS)
  # Parse and check arguments.
  set(OPTIONS SKIP_IWYU SKIP_CLANG_TIDY)
  cmake_parse_arguments(SA "${OPTIONS}" "" "" ${ARGN})
  # Enable IWYU.
  if(NOT SKIP_IWYU)
    check_includes(${TARGET_OR_ALIAS})
  endif()
  # Enable clang-tidy.
  if(NOT SKIP_CLANG_TIDY)
    enable_clang_tidy(${TARGET_OR_ALIAS})
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

## Enable global static analysis.
function(enable_global_static_analysis)
  # Check spelling.
  check_spelling(${CMAKE_PROJECT_NAME} "${CMAKE_SOURCE_DIR}")
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
