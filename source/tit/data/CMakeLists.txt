# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    data
  SOURCES
    "hdf5.cpp"
    "hdf5.hpp"
    "sqlite.cpp"
    "sqlite.hpp"
    "storage.cpp"
    "storage.hpp"
    "type.hpp"
  DEPENDS
    tit::core
    HighFive::HighFive
    sqlite3::sqlite3
    tinyxml2::tinyxml2
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    data_tests
  SOURCES
    "sqlite.test.cpp"
    "storage.test.cpp"
    "type.test.cpp"
  DEPENDS
    tit::data
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
