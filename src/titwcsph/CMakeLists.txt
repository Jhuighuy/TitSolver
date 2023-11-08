# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES
  wcsph.cpp)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Create the library.
add_executable(titwcsph ${CXX_SOURCES})
add_executable(tit::wcsph ALIAS titwcsph)

# Link with the dependant libraries.
target_link_libraries(titwcsph PRIVATE tit::core tit::geom tit::par tit::sph)

# Enable static analysis.
# check_includes(tit::wcsph)
# enable_clang_tidy(tit::wcsph)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
