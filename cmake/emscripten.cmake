# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(clang)
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find the Emscripten compiler executable.
find_program(EMCC_EXE NAMES "emcc" REQUIRED)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add a WebAssembly binary.
#
# WebAssembly binaries are built "from" native targets. All the source files
# are compiled to WebAssembly, corresponding native object files are used to
# track the dependencies (like, for example, to trigger rebuilds when the
# source files or included headers are updated).
#
# WebAssembly build is limited in the following ways:
#
# - In the WASM world, there is no distinction between a library and an
#   executable. All the targets must have `main` function.
#
# - We do not link with any libraries, except for the standard C++ library and
#   Emscripten's own libraries (like embind or GLFW). Therefore, only the
#   header-only parts of the third-party libraries are available. (We may later
#   add support for linking with our own libraries.)
#
# - Exceptions are disabled. Exceptions make the code significantly slower,
#   Emscripten recommends to disable them.
#
function(add_wasm_binary TARGET NATIVE_TARGET_OR_ALIAS)
  if(NOT TARGET)
    message(FATAL_ERROR "WASM target name must be specified.")
  endif()
  if(NOT NATIVE_TARGET_OR_ALIAS)
    message(
      FATAL_ERROR
      "Native target to be recompiled to WASM must be specified."
    )
  endif()

  # Get the original target name if it is an alias.
  get_target_property(NATIVE_TARGET ${NATIVE_TARGET_OR_ALIAS} ALIASED_TARGET)
  if(NOT TARGET ${NATIVE_TARGET})
    set(NATIVE_TARGET ${NATIVE_TARGET_OR_ALIAS})
  endif()

  # Compile options for the emcc.
  get_generated_compile_options(${NATIVE_TARGET} EMCC_COMPILE_OPTIONS)
  list(
    APPEND
    EMCC_COMPILE_OPTIONS
    # Warnings and diagnostics.
    ${CLANG_WARNINGS}
    # Enable C++23, as it is not configured by the compile options.
    -std=c++23
    # Optimization level.
    -O3
    # Disable exceptions, as they make the code significantly slower.
    -fno-exceptions
  )

  # Link options for the emcc.
  set(
    EMCC_LINK_OPTIONS
    # Inherit compile options.
    ${EMCC_COMPILE_OPTIONS}
    # Increase the default stack size and initial memory, allow memory growth.
    -sSTACK_SIZE=32mb
    -sINITIAL_MEMORY=64mb
    -sALLOW_MEMORY_GROWTH
    # Generate a modern ES6 module.
    -sENVIRONMENT=web
    -sMODULARIZE=1
    -sEXPORT_ES6=1
    # Enable WebGL 2.0 (OpenGL ES 3.0).
    -sUSE_WEBGL2=1
    -sFULL_ES3=1
    # A few libraries.
    -lembind
    -lglfw
    -sUSE_GLFW=3
  )

  # Get the binary, source directory and sources of the target.
  get_target_property(TARGET_SOURCES ${NATIVE_TARGET} SOURCES)
  if(NOT TARGET_SOURCES)
    message(WARNING "emcc: no sources found for target ${NATIVE_TARGET}!")
    return()
  endif()
  get_target_property(TARGET_SOURCE_DIR ${NATIVE_TARGET} SOURCE_DIR)
  get_target_property(TARGET_BINARY_DIR ${NATIVE_TARGET} BINARY_DIR)

  # Loop through the target sources and call emcc.
  set(WASM_OBJECT_FILES)
  foreach(SOURCE ${TARGET_SOURCES})
    # Skip non-C/C++ files.
    is_cpp_source("${SOURCE}" SOURCE_IS_CPP)
    if(NOT SOURCE_IS_CPP)
      message(WARNING "emcc: skipping a non C/C++ file: '${SOURCE}'.")
      continue()
    endif()

    # Define an object file name. The native object file is used as a dependency
    # for the WASM object. This is the only way consistent to get the correct
    # dependency chain. `IMPLICIT_DEPENDS` does not work as expected.
    set(
      NATIVE_OBJECT_FILE
      "${TARGET_BINARY_DIR}/CMakeFiles/${NATIVE_TARGET}.dir/${SOURCE}.o"
    )
    set(
      WASM_OBJECT_FILE
      "${TARGET_BINARY_DIR}/CMakeFiles/${NATIVE_TARGET}.dir/${SOURCE}.wasm.o"
    )
    list(APPEND WASM_OBJECT_FILES "${WASM_OBJECT_FILE}")

    # Compile the source file via emcc.
    set(SOURCE_PATH "${TARGET_SOURCE_DIR}/${SOURCE}")
    cmake_path(
      RELATIVE_PATH WASM_OBJECT_FILE
      BASE_DIRECTORY "${CMAKE_BINARY_DIR}"
      OUTPUT_VARIABLE WASM_OBJECT_FILE_RELATIVE_PATH
    )
    add_custom_command(
      COMMENT "Building WASM object ${WASM_OBJECT_FILE_RELATIVE_PATH}"
      OUTPUT "${WASM_OBJECT_FILE}"
      COMMAND
        "${CHRONIC_EXE}"
          "${EMCC_EXE}"
            ${EMCC_COMPILE_OPTIONS}
            -c "${SOURCE_PATH}"
            -o "${WASM_OBJECT_FILE}"
      DEPENDS "${NATIVE_OBJECT_FILE}"
      COMMAND_EXPAND_LISTS # Needed for generator expressions to work.
      VERBATIM
    )
  endforeach()

  # Link the WASM objects via emcc.
  set(TARGET_OUTPUT_JS "${TARGET_BINARY_DIR}/${TARGET}.js")
  set(TARGET_OUTPUT_WASM "${TARGET_BINARY_DIR}/${TARGET}.wasm")
  set(TARGET_OUTPUTS "${TARGET_OUTPUT_JS}" "${TARGET_OUTPUT_WASM}")
  add_custom_command(
    COMMENT "Linking WASM binary ${TARGET}.{js,wasm}"
    OUTPUT ${TARGET_OUTPUTS}
    COMMAND
      "${CHRONIC_EXE}"
        "${EMCC_EXE}"
          ${EMCC_LINK_OPTIONS}
          ${WASM_OBJECT_FILES}
          -o "${TARGET_OUTPUT_JS}"
    DEPENDS ${WASM_OBJECT_FILES}
    COMMAND_EXPAND_LISTS # Needed for generator expressions to work.
    VERBATIM
  )

  # Create a custom target that would build the WASM binary.
  add_custom_target("${TARGET}" ALL DEPENDS ${NATIVE_TARGET} ${TARGET_OUTPUTS})
  set_target_properties(
    "${TARGET}"
    PROPERTIES "WASM_OUTPUTS" "${TARGET_OUTPUTS}"
  )
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Install the WASM binary.
#
function(install_wasm_binary)
  # Parse and check arguments.
  cmake_parse_arguments(INSTALL "" "TARGET;DESTINATION" "" ${ARGN})
  if(NOT INSTALL_TARGET)
     message(FATAL_ERROR "WASM target name must be specified.")
  endif()
  if(NOT INSTALL_DESTINATION)
    message(FATAL_ERROR "Install destination must be specified.")
  endif()

  # Get the output files.
  get_target_property(TARGET_OUTPUTS "${INSTALL_TARGET}" "WASM_OUTPUTS")
  if(NOT TARGET_OUTPUTS)
    message(FATAL_ERROR "Target ${INSTALL_TARGET} is not a WASM target.")
  endif()

  # Install the files.
  install(FILES ${TARGET_OUTPUTS} DESTINATION "${INSTALL_DESTINATION}/")
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
