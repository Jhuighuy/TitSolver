# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Find clang-tidy.
find_program(CLANG_TIDY_PATH NAMES clang-tidy)

# Find chronic (either system one, either our simple implementation).
find_program(CHRONIC_PATH NAMES chronic chronic.sh
             PATHS ${CMAKE_SOURCE_DIR}/build)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

## Analyze source code with clang-tidy.
function(enable_clang_tidy TARGET_OR_ALIAS)
  # Exit early in case clang-tidy was not found.
  if(NOT CLANG_TIDY_PATH)
    return()
  endif()
  # Get the original target name if it is an alias.
  get_target_property(TARGET ${TARGET_OR_ALIAS} ALIASED_TARGET)
  if(NOT TARGET ${TARGET})
    set(TARGET ${TARGET})
  endif()
  # Fill the list of the common clang-tidy arguments.
  set(CLANG_TIDY_ARGS
      -Wall -Wextra -Wno-unknown-pragmas
      -std=c++2b # Enable C++23.
      -DTIT_IWYU=1)
  # Append target's include directories and definitions.
  foreach(PROP INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND CLANG_TIDY_ARGS
         "$<LIST:TRANSFORM,$<TARGET_PROPERTY:${TARGET},${PROP}>,PREPEND,-I>")
  endforeach()
  foreach(PROP COMPILE_DEFINITIONS INTERFACE_COMPILE_DEFINITIONS)
    list(APPEND CLANG_TIDY_ARGS
         "$<LIST:TRANSFORM,$<TARGET_PROPERTY:${TARGET},${PROP}>,PREPEND,-D>")
  endforeach()
  # Loop through the target sources and call clang-tidy.
  get_target_property(TARGET_SOURCE_DIR ${TARGET} SOURCE_DIR)
  get_target_property(TARGET_SOURCES ${TARGET} SOURCES)
  set(ALL_TAGS)
  foreach(SOURCE ${TARGET_SOURCES})
    # Skip non-C/C++ files.
    get_filename_component(SOURCE_EXT ${SOURCE} EXT)
    string(TOLOWER ${SOURCE_EXT} SOURCE_EXT)
    set(CXX_EXTS ".h" ".c" ".hpp" ".cpp")
    if(NOT (${SOURCE_EXT} IN_LIST CXX_EXTS))
       continue()
    endif()
    # Create tag.
    set(TAG ${SOURCE}.tidy_tag)
    list(APPEND ALL_TAGS ${TAG})
    # Execute clang-tidy.
    add_custom_command(
      OUTPUT ${TAG}
      COMMAND ${CHRONIC_PATH} ${CLANG_TIDY_PATH}
              ${TARGET_SOURCE_DIR}/${SOURCE} --quiet --use-color -- ${CLANG_TIDY_ARGS}
      COMMAND touch ${TAG}
      COMMAND_EXPAND_LISTS
      MAIN_DEPENDENCY ${SOURCE}
      IMPLICIT_DEPENDS CXX ${SOURCE}
      COMMENT "Analyzing ${TARGET_SOURCE_DIR}/${SOURCE}")
  endforeach()
  # Create a custom target that should "build" once all checks succeed.
  add_custom_target("${TARGET}_tidy" ALL DEPENDS ${TARGET} ${ALL_TAGS})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
