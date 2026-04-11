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
    target_compile_options(${TARGET} ${VISIBILITY}
      $<$<CONFIG:${CONFIG}>:${CXX_COMPILE_OPTIONS_${CONFIG_UPPER}}>
    )

    # Apply link options.
    target_link_options(${TARGET} ${VISIBILITY}
      $<$<CONFIG:${CONFIG}>:${CXX_LINK_OPTIONS_${CONFIG_UPPER}}>
    )
  endforeach()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Install a target.
#
function(install_tit_target TYPE TARGET DESTINATION)
  install(TARGETS ${TARGET} ${TYPE} DESTINATION "${DESTINATION}")

  if(APPLE)
    set(ORIGIN "@loader_path")
  else()
    set(ORIGIN "$ORIGIN")
  endif()
  set_target_properties(${TARGET}
    PROPERTIES
      INSTALL_RPATH "${ORIGIN}/../lib"
      BUILD_WITH_INSTALL_RPATH TRUE
  )
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add a library.
#
function(add_tit_library)
  # Parse and check arguments.
  cmake_parse_arguments(TARGET
    ""
    "NAME;TYPE;DESTINATION"
    "SOURCES;DEPENDS"
    ${ARGN}
  )
  set(TARGET "${TARGET_NAME}")

  # Setup the library type and visibility parameter.
  if(TARGET_TYPE)
    # Library type is specified.
    set(ALL_TARGET_TYPES INTERFACE OBJECT STATIC SHARED MODULE)
    if(NOT TARGET_TYPE IN_LIST ALL_TARGET_TYPES)
      list(JOIN ALL_TARGET_TYPES ", " ALL_TARGET_TYPES)
      message(FATAL_ERROR "Library type must be one of: ${ALL_TARGET_TYPES}.")
    endif()
  else()
    # Determine library type based on the presence of compilable sources.
    set(TARGET_TYPE INTERFACE)
    if(TARGET_SOURCES)
      foreach(SOURCE ${TARGET_SOURCES})
        get_filename_component(SOURCE_EXT "${SOURCE}" LAST_EXT)
        string(TOLOWER "${SOURCE_EXT}" SOURCE_EXT)
        set(CXX_EXTS ".c" ".cpp" ".cxx" ".c++" ".cc")
        if("${SOURCE_EXT}" IN_LIST CXX_EXTS)
          set(TARGET_TYPE STATIC)
          break()
        endif()
      endforeach()
    endif()
  endif()

  # Create the library.
  add_library(${TARGET} ${TARGET_TYPE} ${TARGET_SOURCES})
  add_library("tit::${TARGET_NAME}" ALIAS ${TARGET})

  # Configure the target.
  if(TARGET_TYPE STREQUAL "INTERFACE")
    set(TARGET_VISIBILITY INTERFACE)
  else()
    set(TARGET_VISIBILITY PUBLIC)
  endif()
  configure_tit_target(${TARGET} ${TARGET_VISIBILITY})
  target_link_libraries(${TARGET} ${TARGET_VISIBILITY} ${TARGET_DEPENDS})

  # Install the library.
  if(TARGET_SOURCES AND NOT SKIP_ANALYSIS)
    enable_clang_tidy(${TARGET})
  endif()

  # Install the library.
  if(TARGET_DESTINATION)
    set(SHARED_TARGET_TYPES SHARED MODULE)
    if(NOT TARGET_TYPE IN_LIST SHARED_TARGET_TYPES)
      list(JOIN SHARED_TARGET_TYPES ", " SHARED_TARGET_TYPES)
      message(FATAL_ERROR
        "Only the library of types: ${SHARED_TARGET_TYPES} can be installed."
      )
    endif()
    install_tit_target(LIBRARY ${TARGET} "${TARGET_DESTINATION}")
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add an executable target.
#
function(add_tit_executable)
  # Parse and check arguments.
  cmake_parse_arguments(TARGET
    ""
    "NAME;DESTINATION"
    "SOURCES;DEPENDS"
    ${ARGN}
  )
  set(TARGET "${TARGET_NAME}")

  # Create the executable.
  add_executable(${TARGET} ${TARGET_SOURCES})
  add_executable("tit::${TARGET}" ALIAS ${TARGET})

  # Configure the target.
  configure_tit_target(${TARGET} PRIVATE)
  target_link_libraries(${TARGET} PRIVATE ${TARGET_DEPENDS})

  # Run linters.
  if(TARGET_SOURCES AND NOT SKIP_ANALYSIS)
    enable_clang_tidy(${TARGET})
  endif()

  # Install the executable.
  if(TARGET_DESTINATION)
    install_tit_target(RUNTIME ${TARGET} "${TARGET_DESTINATION}")
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
