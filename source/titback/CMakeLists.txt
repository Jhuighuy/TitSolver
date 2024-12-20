# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    titback
  DESTINATION
    bin
  SOURCES
    "backend.cpp"
  DEPENDS
    tit::core
    Crow::Crow
    Python3::Python
)

# Install pip.
execute_process(
  COMMAND "${CHRONIC_EXE}" "${Python3_EXECUTABLE}" -m ensurepip --upgrade
  RESULT_VARIABLE PIP_RESULT
)
if(NOT PIP_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to bootstrap pip.")
endif()

# Install Python dependencies.
set(REQUIREMENTS_TXT "${CMAKE_SOURCE_DIR}/requirements.txt")
execute_process(
  COMMAND
    "${CHRONIC_EXE}"
      "${Python3_EXECUTABLE}" -m pip install -r "${REQUIREMENTS_TXT}"
  RESULT_VARIABLE PIP_INSTALL_RESULT
)
if(NOT PIP_INSTALL_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to install Python dependencies.")
endif()

# Copy the Python standard library files.
file(
  GLOB_RECURSE Python3_STDLIB_FILES
  LIST_DIRECTORIES false
  RELATIVE "${Python3_STDLIB}"
  "${Python3_STDLIB}/*"
)
set(INSTALL_DESTINATION "python/lib/python3.10")
foreach(FILE ${Python3_STDLIB_FILES})
  # Skip the source files, configuration files, and tests.
  if("${FILE}" MATCHES "\.py$|\.opt-[12].pyc$|.*/tests?/.*|^config-|^test/")
    continue()
  endif()

  # Bytecode is contained in a `__pycache__` subdirectory with a suffix,
  # we need to strip it.
  get_filename_component(FILE_NAME "${FILE}" NAME)
  get_filename_component(FILE_DIR "${FILE}" DIRECTORY)
  set(FILE_DIR "${INSTALL_DESTINATION}/${FILE_DIR}")
  string(REPLACE "/__pycache__" "/" FILE_DIR "${FILE_DIR}")
  string(REPLACE ".cpython-310." "." FILE_NAME "${FILE_NAME}")
  install(
    FILES "${Python3_STDLIB}/${FILE}"
    DESTINATION "${FILE_DIR}"
    RENAME "${FILE_NAME}"
  )
endforeach()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
