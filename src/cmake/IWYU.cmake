# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Find include-what-you-use.
find_program(IWYU_PATH NAMES include-what-you-use iwyu)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Check `#includes` in the target with include-what-you-use.
function(check_includes TARGET_OR_ALIAS)
  # Exit early in case IWYU was not found.
  if(NOT IWYU_PATH)
    return()
  endif()
  # Get the original target name if it is an alias.
  get_target_property(TARGET ${TARGET_OR_ALIAS} ALIASED_TARGET)
  if(NOT TARGET ${TARGET})
    set(TARGET ${TARGET})
  endif()
  # Fill the list of the common IWYU arguments.
  set(IWYU_ARGS
      -Xiwyu --error
      -Xiwyu --no_fwd_decls
      -Xiwyu --mapping_file=${CMAKE_SOURCE_DIR}/cmake/IWYU.imp
      -std=c++2b # IWYU has no C++23.
      -stdlib=libc++ # Force use G++'s stdlib.
      -DTIT_IWYU=1)
  # Append target's include directories and definitions.
  foreach(PROP INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND IWYU_ARGS
         "$<LIST:TRANSFORM,$<TARGET_PROPERTY:${TARGET},${PROP}>,PREPEND,-I>")
  endforeach()
  foreach(PROP COMPILE_DEFINITIONS INTERFACE_COMPILE_DEFINITIONS)
    list(APPEND IWYU_ARGS
         "$<LIST:TRANSFORM,$<TARGET_PROPERTY:${TARGET},${PROP}>,PREPEND,-D>")
  endforeach()
  # Loop through the target sources and call IWYU.
  get_target_property(TARGET_SOURCE_DIR ${TARGET} SOURCE_DIR)
  get_target_property(TARGET_SOURCES ${TARGET} SOURCES)
  set(ALL_TAGS)
  foreach(SOURCE ${TARGET_SOURCES})
    # Skip non-C/C++ files.
    # get_source_file_property(SOURCE_LANGUAGE ${SOURCE} LANGUAGE)
    # if(NOT ("${SOURCE_LANGUAGE}" STREQUAL "C") AND
    #    NOT ("${SOURCE_LANGUAGE}" STREQUAL "CXX"))
    #    continue()
    # endif()
    # Execute include-what-you-use.
    set(TAG ${SOURCE}.iwuy_tag)
    list(APPEND ALL_TAGS ${TAG})
    add_custom_command(
      OUTPUT ${TAG}
      COMMAND ${IWYU_PATH} ${IWYU_ARGS} ${TARGET_SOURCE_DIR}/${SOURCE}
      COMMAND touch ${TAG}
      COMMAND_EXPAND_LISTS
      MAIN_DEPENDENCY ${SOURCE}
      IMPLICIT_DEPENDS CXX ${SOURCE}
      COMMENT "Checking includes for ${TARGET_SOURCE_DIR}/${SOURCE}")
  endforeach()
  add_custom_target("${TARGET}_iwyu" ALL DEPENDS ${TARGET} ${ALL_TAGS})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
