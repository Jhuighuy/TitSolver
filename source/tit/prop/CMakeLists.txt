# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    prop
  SOURCES
    "path.cpp"
    "path.hpp"
    "spec-json.cpp"
    "spec.cpp"
    "spec.hpp"
    "tree-file.cpp"
    "tree-json.cpp"
    "tree-yaml.cpp"
    "tree.cpp"
    "tree.hpp"
    "unit.cpp"
    "unit.hpp"
    "validation.cpp"
    "validation.hpp"
  DEPENDS
    tit::core
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    prop_tests
  SOURCES
    "path.test.cpp"
    "spec.test.cpp"
    "tree.test.cpp"
    "unit.test.cpp"
    "validation.test.cpp"
  DEPENDS
    tit::prop
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
