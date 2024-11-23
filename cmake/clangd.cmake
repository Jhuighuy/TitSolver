# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(clang)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Write the `compile_flags.txt` for the clangd language server.
#
# TODO: In a better world, we would simply use `CMAKE_EXPORT_COMPILE_COMMANDS`.
#
function(write_compile_flags SAMPLE_TARGET)
  # Setup "compilation" arguments for hypothetical "clang" call.
  set(CLANG_COMPILE_ARGS)
  get_generated_compile_options(${SAMPLE_TARGET} CLANG_COMPILE_ARGS)
  list(
    APPEND
    CLANG_COMPILE_ARGS
    # Inherit compile options we would use for compilation.
    ${CLANG_COMPILE_OPTIONS}
    # Enable C++23, as it is not configured by the compile options.
    -std=c++23
  )

  # Remove `-Werror` from the compile flags, as it breaks clangd.
  list(REMOVE_ITEM CLANG_COMPILE_ARGS "-Werror")

  # Write the compile flags to a file, each on a new line.
  add_custom_target(
    "${CMAKE_PROJECT_NAME}_clangd_compile_flags"
    ALL # Execute on every build.
    COMMAND
      "${CMAKE_COMMAND}" -E echo ${CLANG_COMPILE_ARGS} |
      xargs -n 1 > "${CMAKE_SOURCE_DIR}/compile_flags.txt"
    COMMENT "Writing compile_flags.txt for clangd"
    COMMAND_EXPAND_LISTS # Needed for generator expressions to work.
    VERBATIM
  )
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
