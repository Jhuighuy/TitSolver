# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Find chronic, either system one, either our simple implementation. It is used
# by some analysis tools that does not provide (proper) quiet mode.
find_program(
  CHRONIC_EXE
  NAMES chronic chronic.sh
  PATHS ${CMAKE_SOURCE_DIR}/build
  REQUIRED)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

## Get program version.
## Program version is determined by executing the program with `--version`
## option and searching for the first occurrence of two or more numbers,
## which are separated by a dot symbol.
function(get_program_version PROG_EXE PROG_VERSION_VAR)
  # Check arguments.
  if(NOT PROG_EXE)
    message(FATAL_ERROR "Program path must be specified.")
  endif()
  # Run the executable to get its version.
  execute_process(COMMAND ${PROG_EXE} --version
                  RESULT_VARIABLE PROCESS_RESULT
                  OUTPUT_VARIABLE PROCESS_OUTPUT)
  if(NOT PROCESS_RESULT EQUAL 0)
    message(FATAL_ERROR "Could not execute ${PROG_EXE} --version")
  endif()
  # Extract the full version string.
  string(REGEX MATCH "[0-9]+(\\.[0-9]+)+" PROG_VERSION "${PROCESS_OUTPUT}")
  if(NOT PROG_VERSION)
    message(FATAL_ERROR "Could not find a version number.")
  endif()
  # Propagate the version variable to parent scope.
  set(${PROG_VERSION_VAR} ${PROG_VERSION} PARENT_SCOPE)
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

## Find a program and check whether it's version is sufficient.
## This mostly function replicates interface of the original `find_program`,
## adding the additional mandatory `MIN_VERSION` keyword-prefixed argument.
function(find_program_with_version PROG_EXE_VAR)
  # Parse and check arguments.
  set(OPTIONS REQUIRED NO_CACHE)
  set(ONE_VALUE_ARGS MIN_VERSION)
  set(MULTI_VALUE_ARGS NAMES HINTS PATHS)
  cmake_parse_arguments(
    PROG
    "${OPTIONS}"
    "${ONE_VALUE_ARGS}"
    "${MULTI_VALUE_ARGS}"
    ${ARGN})
  if(NOT PROG_NAMES)
    message(FATAL_ERROR "Program names must be specified.")
  endif()
  if(NOT PROG_MIN_VERSION)
    message(FATAL_ERROR "Program minimal version must be specified.")
  endif()
  # Search for the program. Do not cache the result yet.
  find_program(
    PROG_EXE
    NAMES ${PROG_NAMES}
    HINTS ${PROG_HINTS}
    PATHS ${PROG_PATHS}
    NO_CACHE)
  if(NOT PROG_EXE)
    if(PROG_REQUIRED)
      list(JOIN PROG_NAMES ", " PROG_NAMES)
      message(
        FATAL_ERROR
        "Count not find ${PROG_EXE_VAR} using the following names: "
        ${PROG_NAMES})
    endif()
    return()
  endif()
  # Check program version.
  get_program_version(${PROG_EXE} PROG_VERSION)
  if(PROG_VERSION VERSION_LESS ${PROG_MIN_VERSION})
    if(PROG_REQUIRED)
      set(MESSAGE_LEVEL FATAL_ERROR)
    else()
      set(MESSAGE_LEVEL WARNING)
    endif()
    message(
      ${MESSAGE_LEVEL}
      "Insuffient version of ${PROG_EXE_VAR} was found. "
      "Minimum required version is ${PROG_MIN_VERSION}.")
    return()
  endif()
  # Propagate the program path to parent scope or cache on success.
  if(PROG_NO_CACHE)
    set(${PROG_EXE_VAR} ${PROG_EXE} PARENT_SCOPE)
  else()
    set(${PROG_EXE_VAR} ${PROG_EXE} CACHE INTERNAL "Path to ${PROG_EXE_VAR}")
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

## Determine is source is a C/C++ file based on it's extension.
function(is_cxx_source SOURCE_PATH RESULT_VAR)
  # Get source file extension.
  get_filename_component(SOURCE_EXT ${SOURCE_PATH} EXT)
  string(TOLOWER ${SOURCE_EXT} SOURCE_EXT)
  # Check if extension is in the list of C/C++ extensions and propagate the
  # result to outer scope.
  set(
    VALID_EXTENSIONS
    ".c" ".cpp" ".cxx" ".c++" ".cc"
    ".h" ".hpp" ".hxx" ".h++" ".hh")
  if(SOURCE_EXT IN_LIST VALID_EXTENSIONS)
    set(${RESULT_VAR} TRUE PARENT_SCOPE)
  else()
    set(${RESULT_VAR} FALSE PARENT_SCOPE)
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

## Get a selected subset of the target's compile options (include directories,
## compile definitions). Since most of those are not known at compile time,
## the output list may contain generator expressions. This makes it unusable
## for debugging, but fully suitable for `add_custom_command` for example.
function(get_generated_compile_options TARGET OPTIONS_VAR)
  # Append include directories.
  foreach(PROP INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES)
    list(
      APPEND
      OPTIONS
      "$<LIST:TRANSFORM,$<TARGET_PROPERTY:${TARGET},${PROP}>,PREPEND,-I>")
  endforeach()
  # Append compile definitions.
  foreach(PROP COMPILE_DEFINITIONS INTERFACE_COMPILE_DEFINITIONS)
    list(
      APPEND
      OPTIONS
      "$<LIST:TRANSFORM,$<TARGET_PROPERTY:${TARGET},${PROP}>,PREPEND,-D>")
  endforeach()
  # Propagate the list variable to parent scope.
  set(${OPTIONS_VAR} ${OPTIONS} PARENT_SCOPE)
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

## Read value from the variable by it's name, if it is defined. If it is
## undefined and default values are provided, those are set.
function(try_set VAR VAR_NAME)
  if(DEFINED ${VAR_NAME})
    set(${VAR} ${${VAR_NAME}} PARENT_SCOPE)
  elseif(ARGN)
    set(${VAR} ${ARGN} PARENT_SCOPE)
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
