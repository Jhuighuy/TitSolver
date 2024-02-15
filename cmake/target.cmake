# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(clang_tidy)
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set(TARGET_NAME_PREFIX "tit_")
set(TARGET_ALIAS_NAME_PREFIX "tit::")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Add a library.
function(tit_add_library)
  # Parse and check arguments.
  cmake_parse_arguments(
    LIB
    ""
    "NAME"
    "SOURCES;DEPENDS"
    ${ARGN})
  if(NOT LIB_NAME)
    message(FATAL_ERROR "Library name must be specified.")
  endif()
  # Determine library type.
  set(LIB_HAS_SOURCES FALSE)
  if(LIB_SOURCES)
    foreach(SOURCE ${LIB_SOURCES})
      is_cpp_source("${SOURCE}" LIB_HAS_SOURCES)
      if(LIB_HAS_SOURCES)
        break()
      endif()
    endforeach()
  endif()
  if(LIB_HAS_SOURCES)
    set(LIB_TYPE STATIC)
    set(LIB_PUBLIC PUBLIC)
    set(LIB_PRIVATE PRIVATE)
  else()
    set(LIB_TYPE INTERFACE)
    set(LIB_PUBLIC INTERFACE)
    set(LIB_PRIVATE INTERFACE)
  endif()
  # Create the library.
  set(LIB_TARGET "${TARGET_NAME_PREFIX}${LIB_NAME}")
  set(LIB_TARGET_ALIAS "${TARGET_ALIAS_NAME_PREFIX}${LIB_NAME}")
  add_library(${LIB_TARGET} ${LIB_TYPE} ${LIB_SOURCES})
  add_library(${LIB_TARGET_ALIAS} ALIAS ${LIB_TARGET})
  # Link with the dependent libraries.
  list(TRANSFORM LIB_DEPENDS PREPEND ${TARGET_ALIAS_NAME_PREFIX})
  target_link_libraries(${LIB_TARGET} ${LIB_PUBLIC} ${LIB_DEPENDS})
  # Enable static analysis.
  enable_clang_tidy(${LIB_TARGET})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Add an executable.
function(tit_add_executable)
  # Parse and check arguments.
  cmake_parse_arguments(
    EXE
    ""
    "NAME"
    "SOURCES;DEPENDS"
    ${ARGN})
  if(NOT EXE_NAME)
    message(FATAL_ERROR "Executable name must be specified.")
  endif()
  # Create the executable.
  set(EXE_TARGET "${TARGET_NAME_PREFIX}${EXE_NAME}")
  set(EXE_TARGET_ALIAS "${TARGET_ALIAS_NAME_PREFIX}${EXE_NAME}")
  add_executable(${EXE_TARGET} ${EXE_SOURCES})
  add_executable(${EXE_TARGET_ALIAS} ALIAS ${EXE_TARGET})
  # Link with the dependent libraries.
  list(TRANSFORM EXE_DEPENDS PREPEND ${TARGET_ALIAS_NAME_PREFIX})
  target_link_libraries(${EXE_TARGET} PRIVATE ${EXE_DEPENDS})
  # Enable static analysis.
  enable_clang_tidy(${EXE_TARGET})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
