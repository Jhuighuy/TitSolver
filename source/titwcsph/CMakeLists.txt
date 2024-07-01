# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Specify the list of sources.
set(
  CXX_SOURCES
  "wcsph.cpp"
)

# Create the executable.
add_executable(titwcsph ${CXX_SOURCES})
add_executable(tit::wcsph ALIAS titwcsph)

# Link with the dependent libraries.
target_link_libraries(titwcsph PRIVATE tit::sph)

# Enable static analysis.
# enable_clang_tidy(tit::wcsph)

# Install the executable.
install(TARGETS titwcsph RUNTIME DESTINATION bin)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
