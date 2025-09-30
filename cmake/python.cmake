# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find Python interpreter.
find_program(PYTHON_EXE NAMES "python3" REQUIRED)

# Find the coverage.py executable.
find_program(COVERAGE_EXE NAMES "coverage" REQUIRED)

# Setup Python run command.
set(PYTHON_RUN_CMD "$<IF:$<CONFIG:Coverage>,${COVERAGE_EXE} run,${PYTHON_EXE}>")

# Setup Python path.
set(PYTHON_PATH "${CMAKE_SOURCE_DIR}/source:$ENV{PYTHONPATH}")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find the Flake8 executable.
find_program(FLAKE8_EXE NAMES "flake8")

# Define Flake8 run options.
set(
  FLAKE8_OPTIONS
  "--toml-config=${CMAKE_SOURCE_DIR}/pyproject.toml"
)
list(JOIN FLAKE8_OPTIONS " " FLAKE8_OPTIONS)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find the Pylint executable.
find_program(PYLINT_EXE NAMES "pylint")

# Define Pylint run options.
set(
  PYLINT_OPTIONS
  "--rcfile=${CMAKE_SOURCE_DIR}/pyproject.toml"
  "--score=False"
  "--persistent=False"
  "--output-format=colorized"
)
list(JOIN PYLINT_OPTIONS " " PYLINT_OPTIONS)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find the MyPy executable.
find_program(MYPY_EXE NAMES "mypy")

# Define MyPy run options.
set(
  MYPY_OPTIONS
  "--config-file=${CMAKE_SOURCE_DIR}/pyproject.toml"
)
list(JOIN MYPY_OPTIONS " " MYPY_OPTIONS)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add a Python target.
#
function(add_tit_python_target)
  # Parse and check arguments.
  cmake_parse_arguments(TARGET "" "NAME;DESTINATION" "SOURCES" ${ARGN})
  make_target_name("${TARGET_NAME}" TARGET)

  # Create the target.
  # This will be a no-op target, since we are not compiling Python code anyhow.
  add_custom_target("${TARGET}" ALL DEPENDS ${TARGET_SOURCES})

  # Run linters.
  if(NOT SKIP_ANALYSIS AND (FLAKE8_EXE OR PYLINT_EXE OR MYPY_EXE))
    set(ALL_STAMPS)
    foreach(SOURCE ${TARGET_SOURCES})
      # We'll need a stamp file to track the analysis.
      string(REPLACE "/" "_" STAMP "${SOURCE}.stamp")
      set(STAMP "${CMAKE_CURRENT_BINARY_DIR}/${STAMP}")
      list(APPEND ALL_STAMPS "${STAMP}")

      # Locate the source file.
      if(NOT EXISTS "${SOURCE}")
        set(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}")
        if(NOT EXISTS "${SOURCE}")
          message(WARNING "Source file '${SOURCE}' cannot be found.")
          continue()
        endif()
      endif()

      # Setup the lint command.
      set(LINT_COMMAND)
      if(FLAKE8_EXE)
        list(APPEND LINT_COMMAND "${FLAKE8_EXE} ${SOURCE} ${FLAKE8_OPTIONS}")
      endif()
      if(PYLINT_EXE)
        list(APPEND LINT_COMMAND "${PYLINT_EXE} ${SOURCE} ${PYLINT_OPTIONS}")
      endif()
      if(MYPY_EXE)
        list(APPEND LINT_COMMAND "${MYPY_EXE} ${SOURCE} ${MYPY_OPTIONS}")
      endif()
      list(JOIN LINT_COMMAND " && " LINT_COMMAND)

      # Run the linters and update a stamp file on success.
      add_custom_command(
        COMMENT "Linting ${TARGET}"
        OUTPUT "${STAMP}"
        COMMAND
          "${CMAKE_COMMAND}" -E env "PYTHONPATH=${PYTHON_PATH}"
              "${CHRONIC_EXE}" "${SHELL_EXE}" -c "${LINT_COMMAND}"
        COMMAND
          "${CMAKE_COMMAND}" -E touch "${STAMP}"
        DEPENDS "${SOURCE}" "${CMAKE_SOURCE_DIR}/pyproject.toml"
        VERBATIM
      )
    endforeach()
    add_custom_target("${TARGET}_lint" ALL DEPENDS ${ALL_STAMPS})
  endif()

  # Install the target.
  if(TARGET_DESTINATION)
    install_each_file(
      DESTINATION "${TARGET_DESTINATION}"
      BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
      FILES ${TARGET_SOURCES}
    )
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
