# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    titdata
  DESTINATION
    paraview_plugin
  TYPE
    SHARED
  SOURCES
    "titdata.cpp"
    "titdata.hpp"
  DEPENDS
    tit::data
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# TODO: Add a proper wrapper function for creating Python bindings.
nanobind_add_module(
  _data
  _data.cpp
  NB_STATIC STABLE_ABI
)

target_link_libraries(_data PUBLIC tit::data)

enable_clang_tidy(_data)

install(TARGETS _data LIBRARY DESTINATION "paraview_plugin")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Copy Python files to the ParaView plugin directory.
file(GLOB PYTHON_SOURCES "*.py")
install(FILES ${PYTHON_SOURCES} DESTINATION "paraview_plugin/")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
