# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    prop
  SOURCES
    "app.cpp"
    "app.hpp"
    "spec-info.cpp"
    "spec-json.cpp"
    "spec-yaml.cpp"
    "spec.cpp"
    "spec.hpp"
    "tree-args.cpp"
    "tree-json.cpp"
    "tree-yaml.cpp"
    "tree.cpp"
    "tree.hpp"
  DEPENDS
    tit::core
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
)

add_tit_executable(
  NAME
    prop_tests
  SOURCES
    "spec.test.cpp"
    "tree.test.cpp"
  DEPENDS
    tit::prop
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
