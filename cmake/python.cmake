# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Patterns for the Python standard library files to skip.
set(
  Python3_STDLIB_SKIP_PATTERNS
  # Source files.
  "\.py$"
  # Optimized bytecode files.
  "\.opt-[12]\\.pyc$"
  # Tests.
  "^test/|/tests?/"
  # Configuration files.
  "^config-"
  # `pip` and `setuptools`. We keep `ensurepip`, so the users can re-install
  # `pip` using it and install packages as they wish.
  "^site-packages/pip"
  "^site-packages/setuptools"
  # Some standard library modules that are very unlikely to be used.
  "^lib2to3"
  "^turtle"
)
list(JOIN Python3_STDLIB_SKIP_PATTERNS "|" Python3_STDLIB_SKIP_PATTERNS)

#
# Install the Python standard library and packages.
#
function(install_python)
  # Parse and check arguments.
  cmake_parse_arguments(INSTALL "" "DESTINATION" "PACKAGES" ${ARGN})
  if(NOT INSTALL_DESTINATION)
    message(FATAL_ERROR "Install destination must be specified.")
  endif()

  # Install pip and packages. Packages would be installed into `site-packages`
  # directory of the Python standard library.
  message(STATUS "Running pip install")
  set(VCPKG_BIN_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
  execute_process(
    COMMAND
      "${CMAKE_COMMAND}"
        -E env "PATH=${VCPKG_BIN_DIR}:$PATH" # Suppress warning.
        "${Python3_EXECUTABLE}" -m ensurepip --upgrade
    RESULT_VARIABLE ENSURE_PIP_RESULT
  )
  if(NOT ENSURE_PIP_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to install Pip!")
  endif()
  execute_process(
    COMMAND
      "${CMAKE_COMMAND}"
        -E env "PATH=${VCPKG_BIN_DIR}:$PATH" # Suppress warning.
        "${Python3_EXECUTABLE}" -m pip install --upgrade pip ${INSTALL_PACKAGES}
    RESULT_VARIABLE PIP_RESULT
  )
  if(NOT PIP_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to install Python packages!")
  endif()
  message(STATUS "Running pip install - done")

  # Find the standard library files.
  file(
    GLOB_RECURSE Python3_STDLIB_FILES
    LIST_DIRECTORIES false
    RELATIVE "${Python3_STDLIB}"
    "${Python3_STDLIB}/*"
  )

  # Copy the standard library files. Standard library files must be stored in
  # the `lib/python3.X` directory of a Python home directory.
  foreach(FILE ${Python3_STDLIB_FILES})
    # Skip the source files, configuration files, and tests.
    if("${FILE}" MATCHES "${Python3_STDLIB_SKIP_PATTERNS}")
      continue()
    endif()
    get_filename_component(FILE_NAME "${FILE}" NAME)
    get_filename_component(FILE_DIR "${FILE}" DIRECTORY)

    # Since we are not copying the source files, we need the compiled files to
    # replace them. Compiled files are stored in the `__pycache__` directory,
    # and have a version-specific suffix. We need to remove the suffix from the
    # file name and `__pycache__` from the directory name to make everything
    # work correctly.
    if ("${FILE}" MATCHES "\\.pyc$")
      string(
        REPLACE "__pycache__" ""
        FILE_DIR "${FILE_DIR}"
      )
      string(
        REPLACE ".cpython-3${Python3_VERSION_MINOR}" ""
        FILE_NAME "${FILE_NAME}"
      )
    endif()

    # Install the file.
    install(
      FILES
        "${Python3_STDLIB}/${FILE}"
      DESTINATION
        "${INSTALL_DESTINATION}/lib/python3.${Python3_VERSION_MINOR}/${FILE_DIR}"
      RENAME
        "${FILE_NAME}"
    )
  endforeach()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
