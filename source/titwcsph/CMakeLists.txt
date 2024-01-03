# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES
  "wcsph.cpp")

# Create the executable.
add_executable(titwcsph ${CXX_SOURCES})
add_executable(tit::wcsph ALIAS titwcsph)

# Link with the dependent libraries.
target_link_libraries(titwcsph PRIVATE tit::sph tit::app)

# Enable static analysis.
# check_includes(tit::wcsph)
# enable_clang_tidy(tit::wcsph)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
