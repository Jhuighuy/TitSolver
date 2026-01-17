# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(clang)
include(compiler)
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Set the compile and link options for the given target.
#
function(configure_tit_target TARGET VISIBILITY)
  # Setup include directories.
  target_include_directories(
    ${TARGET} ${VISIBILITY}
    "${CMAKE_SOURCE_DIR}/source"
  )

  # Setup compile and link options.
  foreach(CONFIG ${ALL_CONFIGS})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)

    # Apply compilation options.
    target_compile_options(
      ${TARGET} ${VISIBILITY}
      $<$<CONFIG:${CONFIG}>:${CXX_COMPILE_OPTIONS_${CONFIG_UPPER}}>
    )

    # Apply link options.
    target_link_options(
      ${TARGET} ${VISIBILITY}
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
    TARGET ""
    "NAME;TYPE;DESTINATION"
    "SOURCES;DEPENDS"
    ${ARGN}
  )
  make_target_name(${TARGET_NAME} TARGET)

  # Setup the library type and visibility parameter.
  if(TARGET_TYPE)
    # Library type is specified.
    set(ALL_TARGET_TYPES INTERFACE OBJECT STATIC SHARED)
    if(NOT TARGET_TYPE IN_LIST ALL_TARGET_TYPES)
      list(JOIN ALL_TARGET_TYPES ", " ALL_TARGET_TYPES)
      message(FATAL_ERROR "Library type must be one of: ${ALL_TARGET_TYPES}.")
    endif()
  else()
    # Determine library type based on the presence of compilable sources.
    set(TARGET_HAS_SOURCES FALSE)
    if(TARGET_SOURCES)
      foreach(SOURCE ${TARGET_SOURCES})
        is_cpp_source("${SOURCE}" TARGET_HAS_SOURCES)
        if(TARGET_HAS_SOURCES)
          break()
        endif()
      endforeach()
    endif()
    if(TARGET_HAS_SOURCES)
      set(TARGET_TYPE STATIC)
    else()
      set(TARGET_TYPE INTERFACE)
    endif()
  endif()
  if(TARGET_TYPE STREQUAL "INTERFACE")
    set(TARGET_VISIBILITY INTERFACE)
  else()
    set(TARGET_VISIBILITY PUBLIC)
  endif()

  # Create the library.
  add_library(${TARGET} ${TARGET_TYPE} ${TARGET_SOURCES})
  add_library("${TARGET_NAME_PREFIX}::${TARGET_NAME}" ALIAS ${TARGET})

  # Configure the target.
  configure_tit_target(${TARGET} ${TARGET_VISIBILITY})
  target_link_libraries(${TARGET} ${TARGET_VISIBILITY} ${TARGET_DEPENDS})

  # Install the library.
  if(TARGET_SOURCES AND NOT SKIP_ANALYSIS)
    enable_clang_tidy("${TARGET}")
  endif()

  # Install the library.
  if(TARGET_DESTINATION)
    if(NOT TARGET_TYPE STREQUAL "SHARED")
      message(FATAL_ERROR "Only shared libraries can be installed.")
    endif()
    install(TARGETS ${TARGET} LIBRARY DESTINATION "${TARGET_DESTINATION}")
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add an executable target.
#
function(add_tit_executable)
  # Parse and check arguments.
  cmake_parse_arguments(TARGET "" "NAME;DESTINATION" "SOURCES;DEPENDS" ${ARGN})
  make_target_name("${TARGET_NAME}" TARGET)

  # Create the executable and the alias.
  add_executable("${TARGET}" ${TARGET_SOURCES})

  # Configure the target.
  configure_tit_target("${TARGET}" PRIVATE)
  target_link_libraries("${TARGET}" PRIVATE ${TARGET_DEPENDS})

  # Run linters.
  if(TARGET_SOURCES AND NOT SKIP_ANALYSIS)
    enable_clang_tidy("${TARGET}")
  endif()

  # Install the executable.
  if(TARGET_DESTINATION)
    install(TARGETS "${TARGET}" RUNTIME DESTINATION "${TARGET_DESTINATION}")
  else()
    # TODO: We should not force targets the caller did not ask us for.
    install(TARGETS "${TARGET}" RUNTIME DESTINATION "private/bin")
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
