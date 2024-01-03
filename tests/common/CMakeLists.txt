# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES)

# Create the library.
add_library(tit_tests_common INTERFACE ${CXX_SOURCES})
add_library(tit::tests_common ALIAS tit_tests_common)

# Setup the include directory.
target_include_directories(tit_tests_common INTERFACE "..")

# Link with the dependent libraries.
target_link_libraries(tit_tests_common INTERFACE tit::core)

# Enable static analysis.
enable_static_analysis(tit::tests_common)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
