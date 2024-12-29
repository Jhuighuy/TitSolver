# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Patterns for the Python standard library files to skip.
set(
  Python_STDLIB_SKIP_PATTERNS
  "\.py$"             # Source files.
  "\.opt-[12]\\.pyc$" # Optimized bytecode files.
  "^test/|/tests?/"   # Tests.
  "^config-"          # Configuration files.
)
list(JOIN Python_STDLIB_SKIP_PATTERNS "|" Python_STDLIB_SKIP_PATTERNS)

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
  execute_process(
    COMMAND "${CHRONIC_EXE}" "${Python_EXECUTABLE}" -m ensurepip --upgrade
    RESULT_VARIABLE ENSURE_PIP_RESULT
  )
  if(NOT ENSURE_PIP_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to install Pip!")
  endif()
  execute_process(
    COMMAND
      "${CHRONIC_EXE}"
        "${Python_EXECUTABLE}" -m pip install ${INSTALL_PACKAGES}
    RESULT_VARIABLE PIP_RESULT
  )
  if(NOT PIP_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to install Python packages!")
  endif()

  # Find the standard library files.
  file(
    GLOB_RECURSE Python_STDLIB_FILES
    LIST_DIRECTORIES false
    RELATIVE "${Python_STDLIB}"
    "${Python_STDLIB}/*"
  )

  # Copy the standard library files. Standard library files must be stored in
  # the `lib/python3.X` directory of a Python home directory.
  foreach(FILE ${Python_STDLIB_FILES})
    # Skip the source files, configuration files, and tests.
    if("${FILE}" MATCHES "${Python_STDLIB_SKIP_PATTERNS}")
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
        REPLACE ".cpython-3${Python_VERSION_MINOR}" ""
        FILE_NAME "${FILE_NAME}"
      )
    endif()

    # Install the file.
    install(
      FILES
        "${Python_STDLIB}/${FILE}"
      DESTINATION
        "${INSTALL_DESTINATION}/lib/python3.${Python_VERSION_MINOR}/${FILE_DIR}"
      RENAME
        "${FILE_NAME}"
    )
  endforeach()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
