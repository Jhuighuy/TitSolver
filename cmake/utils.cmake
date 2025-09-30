# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find chronic.
find_program(CHRONIC_EXE NAMES "chronic" REQUIRED)

# Find shell.
find_program(SHELL_EXE NAMES "bash" "zsh" "sh" REQUIRED)

# Find git.
find_program(GIT_EXE NAMES "git" REQUIRED)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Common target name prefix.
set(TARGET_NAME_PREFIX "tit")

#
# Make a target name.
#
function(make_target_name NAME RESULT_VAR)
  if(NAME MATCHES "^${TARGET_NAME_PREFIX}")
    set(${RESULT_VAR} "${NAME}" PARENT_SCOPE)
  else()
    set(${RESULT_VAR} "${TARGET_NAME_PREFIX}_${NAME}" PARENT_SCOPE)
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# A list of the C/C++ source file extensions.
set(CPP_HEADER_EXTENSIONS ".h" ".hpp" ".hxx" ".h++" ".hh")
set(CPP_SOURCE_EXTENSIONS ".c" ".cpp" ".cxx" ".c++" ".cc")

#
# Determine if file is a C/C++ header file based on it's extension.
#
function(is_cpp_header SOURCE_PATH RESULT_VAR)
  get_filename_component(SOURCE_EXTENSION ${SOURCE_PATH} LAST_EXT)
  string(TOLOWER ${SOURCE_EXTENSION} SOURCE_EXTENSION)
  if(SOURCE_EXTENSION IN_LIST CPP_HEADER_EXTENSIONS)
    set(${RESULT_VAR} TRUE PARENT_SCOPE)
  else()
    set(${RESULT_VAR} FALSE PARENT_SCOPE)
  endif()
endfunction()

#
# Determine if file is a C/C++ source file based on it's extension.
#
function(is_cpp_source SOURCE_PATH RESULT_VAR)
  get_filename_component(SOURCE_EXTENSION ${SOURCE_PATH} LAST_EXT)
  string(TOLOWER ${SOURCE_EXTENSION} SOURCE_EXTENSION)
  if(SOURCE_EXTENSION IN_LIST CPP_SOURCE_EXTENSIONS)
    set(${RESULT_VAR} TRUE PARENT_SCOPE)
  else()
    set(${RESULT_VAR} FALSE PARENT_SCOPE)
  endif()
endfunction()

#
# Determine if file is a C/C++ file based on it's extension.
#
function(is_cpp_file SOURCE_PATH RESULT_VAR)
  is_cpp_header(${SOURCE_PATH} IS_HEADER)
  is_cpp_source(${SOURCE_PATH} IS_SOURCE)
  if(IS_HEADER OR IS_SOURCE)
    set(${RESULT_VAR} TRUE PARENT_SCOPE)
  else()
    set(${RESULT_VAR} FALSE PARENT_SCOPE)
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Get a selected subset of the target's compile options: include directories
# and compile definitions.
#
macro(get_generated_compile_options TARGET OPTIONS_VAR)
  # Append include directories.
  foreach(PROP INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES)
    set(TARGET_INCLUDE_DIRS "$<TARGET_PROPERTY:${TARGET},${PROP}>")
    list(
      APPEND
      ${OPTIONS_VAR}
      "$<LIST:TRANSFORM,${TARGET_INCLUDE_DIRS},PREPEND,-I>"
    )
  endforeach()

  # Append compile definitions.
  foreach(PROP COMPILE_DEFINITIONS INTERFACE_COMPILE_DEFINITIONS)
    set(TARGET_DEFS "$<TARGET_PROPERTY:${TARGET},${PROP}>")
    set(TARGET_DEFS "$<FILTER:${TARGET_DEFS},INCLUDE,.+>") # remove stray `;`.
    list(
      APPEND
      ${OPTIONS_VAR}
      "$<LIST:TRANSFORM,${TARGET_DEFS},PREPEND,-D>"
    )
  endforeach()
endmacro()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Install each file in the list.
#
function(install_each_file)
  # Parse and check arguments.
  cmake_parse_arguments(INSTALL "" "DESTINATION;BASE_DIR" "FILES" ${ARGN})
  if(NOT INSTALL_DESTINATION)
    message(FATAL_ERROR "Installation destination must be specified.")
  endif()
  if(NOT INSTALL_BASE_DIR)
    message(FATAL_ERROR "Base directory must be specified.")
  endif()

  # Install the files.
  foreach(FILE ${INSTALL_FILES})
    # Get the directory of the file relative to the output directory,
    # and append it to the installation destination. This is needed to
    # properly handle the hierarchy of the files, otherwise CMake will
    # install all the files in the destination directory in a flat manner.
    get_filename_component(FILE_DIR "${FILE}" DIRECTORY)
    cmake_path(
      RELATIVE_PATH FILE_DIR
      BASE_DIRECTORY "${INSTALL_BASE_DIR}"
      OUTPUT_VARIABLE FILE_DESTINATION_DIR
    )
    if(FILE_DESTINATION_DIR STREQUAL ".")
      set(FILE_DESTINATION_DIR "") # Just to provide a cleaner output.
    endif()
    set(FILE_DESTINATION_DIR "${INSTALL_DESTINATION}/${FILE_DESTINATION_DIR}")

    # Install the file to the destination directory.
    install(FILES "${FILE}" DESTINATION "${FILE_DESTINATION_DIR}")
  endforeach()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
