# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find the Python interpreter executable.
find_program(PYTHON_EXE NAMES "python3" REQUIRED)

# Find the coverage.py executable.
find_program(COVERAGE_EXE NAMES "coverage" REQUIRED)

# Find the Pylint executable.
find_program(PYLINT_EXE NAMES "pylint")
if(NOT PYLINT_EXE)
  message(WARNING "pylint was not found!")
endif()

# Find the MyPy executable.
find_program(MYPY_EXE NAMES "mypy")
if(NOT MYPY_EXE)
  message(WARNING "mypy was not found!")
endif()

# Setup the Python test runner executable.
set(
  PYTHON_TEST_CMDLINE
  "$<IF:$<CONFIG:Coverage>,${COVERAGE_EXE} run,${PYTHON_EXE}>"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add a Python target.
#
function(add_python_target TARGET)
  if(NOT TARGET)
    message(FATAL_ERROR "Sphinx target name must be specified.")
  endif()

  # Add target that would be "built" once any of the sources are updated.
  # We actually don't build anything, we'll use the target to track the sources.
  set(SOURCES ${ARGN})
  add_custom_target("${TARGET}" ALL DEPENDS "${SOURCES}")
  set_target_properties("${TARGET}" PROPERTIES "SOURCES" "${SOURCES}")
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Install the Python target.
#
function(install_python_target)
  # Parse and check arguments.
  cmake_parse_arguments(INSTALL "" "TARGET;DESTINATION" "" ${ARGN})
  if(NOT INSTALL_TARGET)
     message(FATAL_ERROR "Python target name must be specified.")
  endif()
  if(NOT INSTALL_DESTINATION)
    message(FATAL_ERROR "Install destination must be specified.")
  endif()

  # Get the source directory and sources of the target.
  get_target_property(TARGET_SOURCES "${INSTALL_TARGET}" SOURCES)
  get_target_property(TARGET_SOURCE_DIR "${INSTALL_TARGET}" SOURCE_DIR)

  # Install the sources.
  install_each_file(
    DESTINATION "${INSTALL_DESTINATION}"
    BASE_DIR "${TARGET_SOURCE_DIR}"
    FILES ${TARGET_SOURCES}
  )
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Lint the Python target.
#
function(lint_python_target TARGET)
  if(NOT TARGET)
    message(FATAL_ERROR "PNPM target name must be specified.")
  endif()

  # Should we skip analysis?
  if(SKIP_ANALYSIS)
    return()
  endif()

  # Exit early in case both pylint and mypy were not found.
  if(NOT PYLINT_EXE AND NOT MYPY_EXE)
    return()
  endif()

  # Get the binary, source directory and sources of the target.
  get_target_property(TARGET_SOURCES "${TARGET}" SOURCES)
  get_target_property(TARGET_SOURCE_DIR "${TARGET}" SOURCE_DIR)
  get_target_property(TARGET_BINARY_DIR ${TARGET} BINARY_DIR)

  # Loop through the target sources and call the linters.
  set(ALL_STAMPS)
  foreach(SOURCE ${TARGET_SOURCES})
    # We'll need a stamp file to track the analysis.
    string(REPLACE "/" "_" STAMP "${SOURCE}.lint_stamp")
    set(STAMP "${TARGET_BINARY_DIR}/${STAMP}")
    list(APPEND ALL_STAMPS "${STAMP}")

    # Setup path to the source file.
    if(NOT EXISTS "${SOURCE}")
      if(EXISTS "${TARGET_SOURCE_DIR}/${SOURCE}")
        set(SOURCE "${TARGET_SOURCE_DIR}/${SOURCE}")
      elseif(EXISTS "${TARGET_BINARY_DIR}/${SOURCE}")
        set(SOURCE "${TARGET_BINARY_DIR}/${SOURCE}")
      else()
        message(WARNING "Python: source file '${SOURCE}' does not exist!")
        continue()
      endif()
    endif()

    # Setup the lint command.
    set(LINT_COMMAND)
    if(PYLINT_EXE)
      if(LINT_COMMAND)
        list(APPEND LINT_COMMAND "&&")
      endif()
      list(
        APPEND LINT_COMMAND
        "${PYLINT_EXE}"
          "${SOURCE}"
          "--rcfile=${CMAKE_SOURCE_DIR}/pyproject.toml"
      )
    endif()
    if(MYPY_EXE)
      if(LINT_COMMAND)
        list(APPEND LINT_COMMAND "&&")
      endif()
      list(
        APPEND LINT_COMMAND
        "${MYPY_EXE}"
          "${SOURCE}"
          "--config-file=${CMAKE_SOURCE_DIR}/pyproject.toml"
      )
    endif()
    list(JOIN LINT_COMMAND " " LINT_COMMAND)

    # Execute the linters and update a stamp file on success.
    cmake_path(
      RELATIVE_PATH SOURCE
      BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_VARIABLE RELATIVE_SOURCE_PATH
    )
    add_custom_command(
      COMMENT "Linting ${RELATIVE_SOURCE_PATH}"
      OUTPUT "${STAMP}"
      COMMAND
        "${CMAKE_COMMAND}"
          -E env "PYTHONPATH=${CMAKE_SOURCE_DIR}/source:$ENV{PYTHONPATH}"
            "${CHRONIC_EXE}"
              ${BASH_EXE} -c "${LINT_COMMAND}"
      COMMAND
        "${CMAKE_COMMAND}" -E touch "${STAMP}"
      DEPENDS "${SOURCE}" "${CMAKE_SOURCE_DIR}/pyproject.toml"
      VERBATIM
    )
  endforeach()

  # Create a custom target that should "build" once all checks succeed.
  add_custom_target("${TARGET}_lint" ALL DEPENDS ${TARGET} ${ALL_STAMPS})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
