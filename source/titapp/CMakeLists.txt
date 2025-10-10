# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

install(
  PROGRAMS
    "titapp.sh"
  RENAME
    "titapp"
  DESTINATION
    "bin/"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_python_target(
  NAME
    titapp_py
  SOURCES
    "__init__.py"
    "__main__.py"
    "application.py"
    "server.py"
    "window.py"
  DESTINATION
    "lib/titapp"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Compile GUI code as Qt resource file.
#

# Find pyside6-rcc and gen-qrc script.
find_program(PYSIDE6_QCC_EXE NAMES "pyside6-rcc" REQUIRED)
find_program(
  GEN_QRC_EXE
  NAMES "gen-qrc.sh"
  PATHS "${CMAKE_SOURCE_DIR}/build"
  REQUIRED
)

# Locate the GUI target output directory.
set(GUI_TARGET "titfront")
get_target_property(GUI_BINARY_DIR ${GUI_TARGET} BINARY_DIR)
set(GUI_DIR "${GUI_BINARY_DIR}/dist")

# Generate QRC file and convert it to PySide resources file.
#
# NOTE: For some reason, rebuilding of this target is not triggered unless
#       we explicitly depend on both PNPM target and its stamp file.
set(QRC_FILE "${CMAKE_CURRENT_BINARY_DIR}/resources.qrc")
set(RESOURCES_FILE "${CMAKE_CURRENT_BINARY_DIR}/resources.py")
add_custom_command(
  COMMENT "Generating QRC for ${GUI_DIR}"
  COMMAND "${GEN_QRC_EXE}" "--dir" "${GUI_DIR}" "--output" "${QRC_FILE}"
  COMMAND "${PYSIDE6_QCC_EXE}" "${QRC_FILE}" "-o" "${RESOURCES_FILE}"
  OUTPUT "${QRC_FILE}" "${RESOURCES_FILE}"
  DEPENDS "${GUI_TARGET}" "${GUI_BINARY_DIR}/${GUI_TARGET}.stamp"
)
add_custom_target(
  titapp_resources
  ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/resources.py"
)

# Install the generated resources file.
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/resources.py"
  DESTINATION
    "lib/titapp"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
