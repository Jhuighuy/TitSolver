# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(CXX_SOURCES
    bbox.hpp
    grid.hpp
    hilbert_ordering.hpp
    kd_tree.hpp
    search_engine.hpp)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Create the library.
add_library(tit_geom INTERFACE ${CXX_SOURCES})
add_library(tit::geom ALIAS tit_geom)

# Link with the dependant libraries.
target_link_libraries(tit_geom INTERFACE tit::core tit::par)

# Enable static analysis.
check_includes(tit::geom)
enable_clang_tidy(tit::geom)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
