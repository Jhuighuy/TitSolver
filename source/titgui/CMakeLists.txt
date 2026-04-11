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
    "main/bindings/addon.cpp"
    "main/bindings/storage.cpp"
    "main/bindings/storage.hpp"
    "main/bindings/utils.hpp"
  DEPENDS
    tit::core
    tit::data
    unofficial::node-addon-api::node-addon-api
)

target_compile_definitions(titgui_bindings PRIVATE NAPI_CPP_EXCEPTIONS)

# Set the output name and location.
set_target_properties(titgui_bindings
  PROPERTIES
    OUTPUT_NAME "titgui-bindings"
    PREFIX ""
    SUFFIX ".node"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/native"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Add the Electron app.
add_tit_node_package(
  NAME
    titgui
  DEPENDS
    titgui_bindings
)

# Electron uses `productName` from `package.json` to name the bundle.
file(READ "package.json" PACKAGE_JSON)
string(JSON PRODUCT_NAME GET "${PACKAGE_JSON}" "productName")

# TODO: We would want to create symlinks to the executables into `bin/`.
if(APPLE)
  cmake_path(SET DIST_DIR "dist/${PRODUCT_NAME}-darwin-arm64")

  # Install the bundle to the root.
  install(
    DIRECTORY "${DIST_DIR}/${PRODUCT_NAME}.app"
    DESTINATION "." # Cannot be empty.
    USE_SOURCE_PERMISSIONS
  )
elseif(LINUX)
  cmake_path(SET DIST_DIR "dist/${PRODUCT_NAME}-linux-x64")

  # Install the bundle to `lib/gui`, but do not copy license files.
  install(
    DIRECTORY "${DIST_DIR}"
    DESTINATION "lib/gui"
    USE_SOURCE_PERMISSIONS
    PATTERN "LICENSE*" EXCLUDE
  )
else()
  message(FATAL_ERROR "Unsupported platform.")
endif()

# Install license files.
install(
  FILES "${DIST_DIR}/LICENSE" "${DIST_DIR}/LICENSES.chromium.html"
  DESTINATION "licenses/electron/"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
