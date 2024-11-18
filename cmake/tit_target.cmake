# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(clang_tidy)
include(compiler)
include(sphinx)
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Common target name prefix.
set(TARGET_NAME_PREFIX "tit")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Set the compile and link options for the given target.
#
function(configure_tit_target TARGET ACCESS)
  # Setup include directories.
  target_include_directories(
    ${TARGET} ${ACCESS}
    "${CMAKE_SOURCE_DIR}/source"
  )

  # Setup compile and link options.
  target_compile_features(${TARGET} ${ACCESS} cxx_std_23)
  foreach(CONFIG ${ALL_CONFIGS})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)

    # Apply compilation options.
    target_compile_options(
      ${TARGET} ${ACCESS}
      $<$<CONFIG:${CONFIG}>:${CXX_COMPILE_OPTIONS_${CONFIG_UPPER}}>
    )

    # Apply link options.
    target_link_options(
      ${TARGET} ${ACCESS}
      $<$<CONFIG:${CONFIG}>:${CXX_LINK_OPTIONS_${CONFIG_UPPER}}>
    )
  endforeach()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add a library.
#
function(add_tit_library)
  # Parse and check arguments.
  cmake_parse_arguments(
    LIB
    ""
    "NAME;TYPE;DESTINATION"
    "SOURCES;DEPENDS"
    ${ARGN}
  )
  if(NOT LIB_NAME)
    message(FATAL_ERROR "Library name must be specified.")
  endif()

  if(LIB_TYPE)
    # Library type is specified.
    set(ALL_LIB_TYPES INTERFACE OBJECT STATIC SHARED)
    if(NOT (LIB_TYPE IN_LIST ALL_LIB_TYPES))
      list(JOIN ALL_LIB_TYPES ", " ALL_LIB_TYPES)
      message(
        FATAL_ERROR
        "Library type can be one of the following options: ${ALL_LIB_TYPES}."
      )
    endif()
  else()
    # Determine library type based on the presence of compilable sources.
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
    else()
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
  if(LIB_NAME MATCHES "^${TARGET_NAME_PREFIX}")
    set(LIB_TARGET "${LIB_NAME}")
  else()
    set(LIB_TARGET "${TARGET_NAME_PREFIX}_${LIB_NAME}")
  endif()
  set(LIB_TARGET_ALIAS "${TARGET_NAME_PREFIX}::${LIB_NAME}")
  add_library(${LIB_TARGET} ${LIB_TYPE} ${LIB_SOURCES})
  add_library(${LIB_TARGET_ALIAS} ALIAS ${LIB_TARGET})

  # Configure the target.
  configure_tit_target(${LIB_TARGET} ${LIB_PUBLIC})

  # Link with the dependent libraries.
  target_link_libraries(${LIB_TARGET} ${LIB_PUBLIC} ${LIB_DEPENDS})

  # Install the library.
  if(LIB_DESTINATION)
    set(INSTALLABLE_LIB_TYPES STATIC SHARED)
    if(NOT LIB_TYPE IN_LIST INSTALLABLE_LIB_TYPES)
      message(FATAL_ERROR "Cannot install library of type: ${LIB_TYPE}!")
    endif()
    install(TARGETS ${LIB_TARGET} LIBRARY DESTINATION "${LIB_DESTINATION}")
  endif()

  # Enable static analysis.
  enable_clang_tidy(${LIB_TARGET})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add an executable.
#
function(add_tit_executable)
  # Parse and check arguments.
  cmake_parse_arguments(
    EXE
    ""
    "NAME;PREFIX;DESTINATION"
    "SOURCES;DEPENDS"
    ${ARGN}
  )
  if(NOT EXE_NAME)
    message(FATAL_ERROR "Executable name must be specified.")
  endif()

  # Create the executable and the alias.
  if(EXE_NAME MATCHES "^${TARGET_NAME_PREFIX}")
    set(EXE_TARGET "${EXE_NAME}")
  else()
    set(EXE_TARGET "${TARGET_NAME_PREFIX}_${EXE_NAME}")
  endif()
  set(EXE_TARGET_ALIAS "${TARGET_NAME_PREFIX}::${EXE_NAME}")
  add_executable(${EXE_TARGET} ${EXE_SOURCES})
  add_executable(${EXE_TARGET_ALIAS} ALIAS ${EXE_TARGET})

  # Configure the target.
  configure_tit_target(${EXE_TARGET} PRIVATE)

  # Link with the dependent libraries.
  target_link_libraries(${EXE_TARGET} PRIVATE ${EXE_DEPENDS})

  # Install the executable.
  if(EXE_DESTINATION)
    install(TARGETS ${EXE_TARGET} RUNTIME DESTINATION "${EXE_DESTINATION}")
  endif()

  # Enable static analysis.
  enable_clang_tidy(${EXE_TARGET})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add a documentation target.
#
function(add_tit_documentation)
  # Parse and check arguments.
  cmake_parse_arguments(DOC "" "NAME;DESTINATION" "" ${ARGN})
  if(NOT DOC_NAME)
    message(FATAL_ERROR "Documentation name must be specified.")
  endif()

  # Create the target.
  set(DOC_TARGET "${TARGET_NAME_PREFIX}_${MAN_NAME}")
  add_sphinx_target(${DOC_TARGET})

  # Install the target.
  if(DOC_DESTINATION)
    install_sphinx_target(TARGET ${DOC_TARGET} DESTINATION "${DOC_DESTINATION}")
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
