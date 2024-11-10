# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find chronic, either system one, either our simple implementation. It is used
# by some analysis tools that does not provide (proper) quiet mode.
find_program(
  CHRONIC_EXE
  NAMES "chronic" "chronic.sh"
  PATHS "${CMAKE_SOURCE_DIR}/build"
  REQUIRED
)

# Find bash (or zsh).
find_program(BASH_EXE NAMES "bash" "zsh" REQUIRED)

# Find git.
find_program(GIT_EXE NAMES "git" REQUIRED)

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
    set(TARGET_VALID_DEFS "$<FILTER:${TARGET_DEFS},INCLUDE,\\w+(=\\w+)?>")
    list(
      APPEND
      ${OPTIONS_VAR}
      "$<LIST:TRANSFORM,${TARGET_VALID_DEFS},PREPEND,-D>"
    )
  endforeach()
endmacro()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Read value from the variable by it's name, if it is defined.
# If it is undefined and default values are provided, those are set.
#
macro(try_set VAR VAR_NAME)
  if(DEFINED ${VAR_NAME})
    set(${VAR} ${${VAR_NAME}})
  elseif(ARGN)
    set(${VAR} ${${ARGN}})
  endif()
endmacro()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
