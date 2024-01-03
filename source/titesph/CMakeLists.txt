# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES
  "esph.cpp")

# Create the executable.
add_executable(titesph ${CXX_SOURCES})
add_executable(tit::esph ALIAS titesph)

# Link with the dependent libraries.
target_link_libraries(titesph PRIVATE tit::core tit::sph tit::app)

# Enable static analysis.
# enable_static_analysis(tit::esph)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
