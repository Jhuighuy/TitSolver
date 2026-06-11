# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_subdirectory("bindings")

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

# Install the bundle.
if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
  set(ARCH_SUFFIX "arm64")
else()
  set(ARCH_SUFFIX "x64")
endif()
if(APPLE)
  # Bundle is installed to the root.
  cmake_path(SET DIST_DIR "dist/${PRODUCT_NAME}-darwin-${ARCH_SUFFIX}")
  install(
    DIRECTORY "${DIST_DIR}/${PRODUCT_NAME}.app"
    DESTINATION "." # Cannot be empty.
    USE_SOURCE_PERMISSIONS
  )
elseif(LINUX)
  # Bundle is installed to `lib/gui`.
  cmake_path(SET DIST_DIR "dist/${PRODUCT_NAME}-linux-${ARCH_SUFFIX}")
  install(
    DIRECTORY "${DIST_DIR}/"
    DESTINATION "lib/gui"
    USE_SOURCE_PERMISSIONS
    PATTERN "LICENSE*" EXCLUDE # Installed separately below.
  )
else()
  message(FATAL_ERROR "Unsupported platform.")
endif()

# Install license files.
install(
  FILES "${DIST_DIR}/LICENSE" "${DIST_DIR}/LICENSES.chromium.html"
  DESTINATION "licenses/electron/"
)

# Install launcher script.
if(APPLE)
  set(REL_PATH_TO_EXE "../${PRODUCT_NAME}.app/Contents/MacOS/${PRODUCT_NAME}")
elseif(LINUX)
  set(REL_PATH_TO_EXE "../lib/gui/${PRODUCT_NAME}")
endif()
configure_file("launcher.sh.in" "launcher.sh" @ONLY)
install(
  PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/launcher.sh"
  DESTINATION "bin/"
  RENAME "titgui"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
