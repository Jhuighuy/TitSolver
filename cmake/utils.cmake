# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find chronic.
find_program(CHRONIC_EXE NAMES "chronic" REQUIRED)

# Find shell.
find_program(SHELL_EXE NAMES "bash" "zsh" "sh" REQUIRED)

# Find xargs.
find_program(XARGS_EXE NAMES "xargs" REQUIRED)

# Find git.
find_program(GIT_EXE NAMES "git" REQUIRED)

if(APPLE)
  # Find iconutil.
  find_program(ICONUTIL_EXE NAMES "iconutil" REQUIRED)

  # Find rsvg-convert.
  find_program(RSVG_CONVERT_EXE NAMES "rsvg-convert" REQUIRED)
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Common target name prefix.
set(TARGET_NAME_PREFIX "tit")

#
# Make a target name.
#
function(make_target_name NAME RESULT_VAR)
  if(NOT NAME)
    message(FATAL_ERROR "Target name must be specified.")
  endif()
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
    set(TARGET_INCLUDE_DIRS "$<LIST:SORT,${TARGET_INCLUDE_DIRS}>")
    set(TARGET_INCLUDE_DIRS "$<REMOVE_DUPLICATES:${TARGET_INCLUDE_DIRS}>")
    list(
      APPEND
      ${OPTIONS_VAR}
      "$<LIST:TRANSFORM,${TARGET_INCLUDE_DIRS},PREPEND,-I>"
    )
  endforeach()

  # Append compile definitions.
  foreach(PROP COMPILE_DEFINITIONS INTERFACE_COMPILE_DEFINITIONS)
    set(TARGET_DEFS "$<TARGET_PROPERTY:${TARGET},${PROP}>")
    set(TARGET_DEFS "$<LIST:SORT,${TARGET_DEFS}>")
    set(TARGET_DEFS "$<REMOVE_DUPLICATES:${TARGET_DEFS}>")
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

if(APPLE)
  #
  # Convert an SVG file into a `.icns` file.
  #
  function(convert_svg_to_icns SVG_PATH ICNS_PATH)
    # Get the icon name without the extension.
    get_filename_component(ICON_NAME "${ICNS_PATH}" NAME_WE)

    # Create the `.iconset` directory.
    set(ICONSET_DIR "${CMAKE_CURRENT_BINARY_DIR}/${ICON_NAME}.iconset")
    add_custom_command(
      OUTPUT "${ICONSET_DIR}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${ICONSET_DIR}"
    )

    # Fill the `.iconset` directory with the generated PNGs.
    set(ICONSET_FILES "")
    foreach(SIZE 16 32 128 256 512)
      foreach(SCALE 1 2)
        math(EXPR PIXELS "${SIZE} * ${SCALE}")

        if(${SCALE} EQUAL 1)
          set(ICON "icon_${SIZE}x${SIZE}.png")
        else()
          set(ICON "icon_${SIZE}x${SIZE}@${SCALE}x.png")
        endif()

        set(PNG_PATH "${ICONSET_DIR}/${ICON}")
        list(APPEND ICONSET_FILES "${PNG_PATH}")

        add_custom_command(
          COMMENT "Converting ${SVG_PATH} to ${ICON}"
          OUTPUT "${PNG_PATH}"
          COMMAND
            "${RSVG_CONVERT_EXE}"
              -w "${PIXELS}" -h "${PIXELS}"
              -o "${PNG_PATH}"
              "${CMAKE_CURRENT_SOURCE_DIR}/${SVG_PATH}"
          DEPENDS "${SVG_PATH}" "${ICONSET_DIR}"
        )
      endforeach()
    endforeach()

    # Generate the `.icns` file.
    add_custom_command(
      COMMENT "Generating ${ICNS_PATH}"
      OUTPUT "${ICNS_PATH}"
      COMMAND "${ICONUTIL_EXE}" -c icns "${ICONSET_DIR}" -o "${ICNS_PATH}"
      DEPENDS ${ICONSET_FILES}
    )

    # Add a target to build the `.icns` file.
    add_custom_target(${ICON_NAME}_to_icns ALL DEPENDS "${ICNS_PATH}")
  endfunction()
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
