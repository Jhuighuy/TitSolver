# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_node_package(NAME titgui)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Electron uses `productName` from `package.json` to name the app.
file(READ "package.json" PACKAGE_JSON)
string(JSON PRODUCT_NAME GET "${PACKAGE_JSON}" "productName")

# TODO: We would want to create symlinks to the executables into `bin/`.
if(APPLE)
  cmake_path(SET DIST_DIR "dist/${PRODUCT_NAME}-darwin-arm64")

  # Install the app bundle to the installation root.
  install(
    DIRECTORY "${DIST_DIR}/${PRODUCT_NAME}.app"
    DESTINATION "." # Cannot be empty.
    USE_SOURCE_PERMISSIONS
  )
elseif(LINUX)
  cmake_path(SET DIST_DIR "dist/${PRODUCT_NAME}-linux-x64")

  # Install the app directory to `lib/gui`, but do not copy license files.
  install(
    DIRECTORY "${DIST_DIR}"
    DESTINATION "lib/gui"
    PATTERN "LICENSE*" EXCLUDE
    USE_SOURCE_PERMISSIONS
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
