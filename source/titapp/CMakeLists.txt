# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

qt_standard_project_setup()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  PUBLIC
  NAME
    titapp
  SOURCES
    "app.cpp"
  DEPENDS
    tit::core
    tit::data
    tit::main
    Qt::Core
    Qt::Gui
    Qt::WebEngineWidgets
    Qt::WebSockets
    Qt::Widgets
)

get_target_property(FRONT_OUTPUT_DIR titfront "PNPM_OUTPUT_DIR")
file(GLOB_RECURSE WEB_RESOURCES "${FRONT_OUTPUT_DIR}/*")
qt_add_resources(titapp "resources"
    PREFIX "/"
    BASE "${FRONT_OUTPUT_DIR}"
    FILES ${WEB_RESOURCES}
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
