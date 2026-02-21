# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

qt_standard_project_setup()

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

  install(
    FILES "Icon.icns"
    DESTINATION "BlueTit.app/Contents/Resources"
  )
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
