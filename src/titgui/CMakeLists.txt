# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Find Qt.
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
if(NOT Qt6_FOUND)
  return()
endif()

# Setup Qt.
qt_standard_project_setup()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(CXX_SOURCES
    main_window.cpp
    main_window.hpp
    main_window.ui
    main.cpp)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Create the executable.
qt_add_executable(titgui ${CXX_SOURCES})
add_executable(tit::gui ALIAS titgui)

target_link_libraries(titgui PRIVATE tit::core Qt6::Widgets)

# Enable static analysis.
# check_includes(tit::gui)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
