# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES
  "main_window.cpp"
  "main_window.hpp"
  "main_window.ui"
  "main.cpp")

# Setup Qt. Doing it here in order to avoid running MOC and UIC for all targets.
if(Qt6 VERSION_GREATER_EQUAL 3.3)
  qt_standard_project_setup()
else()
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)
endif()

# Create the executable.
qt_add_executable(titgui ${CXX_SOURCES})
add_executable(tit::gui ALIAS titgui)

# Link with the dependent libraries.
target_link_libraries(titgui PRIVATE tit::app Qt6::Widgets)

# Enable static analysis.
# enable_static_analysis(tit::gui)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
