# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    titsdk
  TYPE
    SHARED
  SOURCES
    "_utils.hpp"
    "lib.cpp"
    "lib.hpp"
    "titsdk.hpp"
    "ttdb.cpp"
    "ttdb.hpp"
  DEPENDS
    tit::data
  DESTINATION
    "lib/titsdk"
)

add_tit_doxygen_target(
  NAME
    titsdk_doxygen
  SOURCES
    "lib.hpp"
    "ttdb.hpp"
)

install(
  FILES
    "lib.hpp"
    "titsdk.hpp"
    "ttdb.hpp"
  DESTINATION
    "lib/titsdk"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_python_target(
  NAME
    titsdk_py
  SOURCES
    "__init__.py"
    "lib.py"
    "ttdb.py"
  DESTINATION
    "lib/titsdk"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
