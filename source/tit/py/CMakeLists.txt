# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    py
  SOURCES
    "_python.hpp"
  DEPENDS
    tit::core
)

# Since we are not linking to Python here, some manual configuration is needed.
target_include_directories(
  tit_py
  INTERFACE
    "${Python3_INCLUDE_DIRS}"
)
target_compile_options(
  tit_py
  INTERFACE
    "-Wno-old-style-cast"
    "-Wno-redundant-decls"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    py_embed
  SOURCES
    "interpreter.cpp"
    "interpreter.hpp"
  DEPENDS
    tit::core
    tit::py
    Python3::Python
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    py_module
  DEPENDS
    tit::py
    Python3::Module
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    py_tests
  SOURCES
  DEPENDS
    tit::py_embed
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
