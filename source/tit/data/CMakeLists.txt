# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    data
  SOURCES
    "export-hdf5.cpp"
    "export-hdf5.hpp"
    "json.hpp"
    "param_rw.hpp"
    "param_spec.cpp"
    "param_spec.hpp"
    "sqlite.cpp"
    "sqlite.hpp"
    "storage.cpp"
    "storage.hpp"
    "type.hpp"
    "zip.cpp"
    "zip.hpp"
    "zstd.cpp"
    "zstd.hpp"
  DEPENDS
    tit::core
    HighFive::HighFive
    miniz::miniz
    nlohmann_json::nlohmann_json
    tinyxml2::tinyxml2
    unofficial::sqlite3::sqlite3
    zstd::libzstd
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    data_tests
  SOURCES
    "sqlite.test.cpp"
    "storage.test.cpp"
    "type.test.cpp"
    "zstd.test.cpp"
  DEPENDS
    tit::data
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
