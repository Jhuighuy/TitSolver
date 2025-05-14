# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

qt_standard_project_setup()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  PUBLIC
  NAME
    titback
  SOURCES
    "backend.cpp"
  DEPENDS
    tit::core
    tit::data
    Qt::Core
    Qt::Gui
    Qt::WebEngineWidgets
    Qt::WebSockets
    Qt::Widgets
)

get_target_property(FRONT_OUTPUT_DIR titfront "PNPM_OUTPUT_DIR")
file(GLOB_RECURSE WEB_RESOURCES "${FRONT_OUTPUT_DIR}/*")
qt_add_resources(titback "resources"
    PREFIX "/"
    BASE "${FRONT_OUTPUT_DIR}"
    FILES ${WEB_RESOURCES}
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
