# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Add the native addon.
add_tit_library(
  NAME
    titgui_bindings
  TYPE
    MODULE
  SOURCES
    "addon.cpp"
    "case.cpp"
    "case.hpp"
    "storage.cpp"
    "storage.hpp"
    "utils.hpp"
  DEPENDS
    tit::core
    tit::data
    tit::prop
    tit::titwcsph_case
    nlohmann_json::nlohmann_json
    node-addon-api::node-addon-api
)

target_compile_definitions(titgui_bindings PRIVATE NAPI_CPP_EXCEPTIONS)

# Set the output name and location.
set_target_properties(titgui_bindings
  PROPERTIES
    OUTPUT_NAME "titgui-bindings"
    PREFIX ""
    SUFFIX ".node"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../native"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
