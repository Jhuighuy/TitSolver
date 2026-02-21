# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

qt_standard_project_setup()

set(Qt6_ROOT_DIR "${Qt6_DIR}/../../../")
set(Qt6_LIB_DIR "${Qt6_ROOT_DIR}/lib")
if(EXISTS "${Qt6_ROOT_DIR}/plugins")
  set(Qt6_PLUGINS_DIR "${Qt6_ROOT_DIR}/plugins")
else()
  # Homebrew on macOS puts plugins in a different location.
  set(Qt6_PLUGINS_DIR "${Qt6_ROOT_DIR}/share/qt/plugins")
endif()

foreach(
  FRAMEWORK
  "QtCore.framework"
  "QtDBus.framework"
  "QtGui.framework"
  "QtNetwork.framework"
  "QtOpenGL.framework"
  "QtPositioning.framework"
  "QtPrintSupport.framework"
  "QtQml.framework"
  "QtQmlMeta.framework"
  "QtQmlModels.framework"
  "QtQmlWorkerScript.framework"
  "QtQuick.framework"
  "QtQuickWidgets.framework"
  "QtWebChannel.framework"
  "QtWebEngineCore.framework"
  "QtWebEngineWidgets.framework"
  "QtWebSockets.framework"
  "QtWidgets.framework"
)
  install(
    DIRECTORY "${Qt6_LIB_DIR}/${FRAMEWORK}"
    DESTINATION "lib"
    USE_SOURCE_PERMISSIONS
  )
endforeach()

install(
  DIRECTORY "${Qt6_PLUGINS_DIR}/"
  DESTINATION "plugins"
  USE_SOURCE_PERMISSIONS
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    titapp
  SOURCES
    "main.cpp"
    "server.cpp"
    "server.hpp"
    "window.cpp"
    "window.hpp"
  DEPENDS
    tit::core
    tit::data
    Qt6::Core
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebSockets
  DESTINATION
    "bin"
)

write_compile_flags(titapp)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

if(APPLE)
  add_tit_executable(
    NAME
      launcher
    SOURCES
      "launcher.cpp"
    DEPENDS
      "-framework AppKit"
      "-framework Foundation"
    DESTINATION
      "BlueTit.app/Contents/MacOS"
  )

  configure_file("Info.plist.in" "Info.plist" @ONLY)
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
    DESTINATION "BlueTit.app/Contents"
  )

  convert_svg_to_icns("Icon.svg" "Icon.icns")
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/Icon.icns"
    DESTINATION "BlueTit.app/Contents/Resources"
  )
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
