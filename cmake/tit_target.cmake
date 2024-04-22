# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(clang_tidy)
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set(TARGET_NAME_PREFIX "tit")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

##
## Add a library.
##
function(add_tit_library)
  # Parse and check arguments.
  cmake_parse_arguments(
    LIB
    ""
    "NAME;TYPE"
    "SOURCES;DEPENDS"
    ${ARGN})
  if(NOT LIB_NAME)
    message(FATAL_ERROR "Library name must be specified.")
  endif()
  # Determine library type.
  if(LIB_TYPE)
    set(ALL_LIB_TYPES INTERFACE OBJECT STATIC SHARED)
    if(NOT (LIB_TYPE IN_LIST ALL_LIB_TYPES))
      list(JOIN ALL_LIB_TYPES ", " ALL_LIB_TYPES)
      message(
        FATAL_ERROR
        "Library type can be one of the following options: ${ALL_LIB_TYPES}.")
    endif()
  else()
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
      # Out target is a static library.
      set(LIB_TYPE STATIC)
    else()
      # Out target is an interface library.
      set(LIB_TYPE INTERFACE)
    endif()
  endif()
  # Setup visibility.
  if(LIB_TYPE STREQUAL "INTERFACE")
    set(LIB_PUBLIC INTERFACE)
  else()
    set(LIB_PUBLIC PUBLIC)
  endif()
  # Create the library and the alias.
  set(LIB_TARGET "${TARGET_NAME_PREFIX}_${LIB_NAME}")
  set(LIB_TARGET_ALIAS "${TARGET_NAME_PREFIX}::${LIB_NAME}")
  add_library(${LIB_TARGET} ${LIB_TYPE} ${LIB_SOURCES})
  add_library(${LIB_TARGET_ALIAS} ALIAS ${LIB_TARGET})
  # Link with the dependent libraries.
  target_link_libraries(${LIB_TARGET} ${LIB_PUBLIC} ${LIB_DEPENDS})
  # Enable static analysis.
  enable_clang_tidy(${LIB_TARGET})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

##
## Add an executable.
##
function(add_tit_executable)
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
  # Create the executable and the alias.
  set(EXE_TARGET "${TARGET_NAME_PREFIX}_${EXE_NAME}")
  set(EXE_TARGET_ALIAS "${TARGET_NAME_PREFIX}::${EXE_NAME}")
  add_executable(${EXE_TARGET} ${EXE_SOURCES})
  add_executable(${EXE_TARGET_ALIAS} ALIAS ${EXE_TARGET})
  # Link with the dependent libraries.
  target_link_libraries(${EXE_TARGET} PRIVATE ${EXE_DEPENDS})
  # Enable static analysis.
  enable_clang_tidy(${EXE_TARGET})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
