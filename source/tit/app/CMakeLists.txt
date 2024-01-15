# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES
  "wrap_main.cpp"
  "wrap_main.hpp")

# Create the library.
add_library(tit_app STATIC ${CXX_SOURCES})
add_library(tit::app ALIAS tit_app)

# Link with the dependent libraries.
target_link_libraries(tit_app PUBLIC tit::core tit::par)

# Enable static analysis.
enable_clang_tidy(tit::app)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
