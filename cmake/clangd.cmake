# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(clang)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Write the `compile_flags.txt` for the clangd language server.
function(write_compile_flags TARGET_OR_ALIAS)
  # Get the original target name if it is an alias.
  get_target_property(TARGET ${TARGET_OR_ALIAS} ALIASED_TARGET)
  if(NOT TARGET ${TARGET})
    set(TARGET ${TARGET_OR_ALIAS})
  endif()

  # Setup "compilation" arguments for mock "clang" call.
  set(CLANG_COMPILE_ARGS)
  get_generated_compile_options(${TARGET} CLANG_COMPILE_ARGS)
  list(
    APPEND
    CLANG_COMPILE_ARGS
    # Enable the same set of compile options we would use for normal clang call.
    ${CLANG_COMPILE_OPTIONS}
    # Enable C++23.
    -std=c++23
  )

  # Write the compile flags to a file, each on a new line.
  add_custom_target(
    "${TARGET}_clangd_compile_flags"
    ALL
    COMMAND
      "${CMAKE_COMMAND}" -E echo ${CLANG_COMPILE_ARGS} |
      xargs -n 1 > "${CMAKE_CURRENT_SOURCE_DIR}/compile_flags.txt"
    COMMENT "Writing compile_flags.txt"
    COMMAND_EXPAND_LISTS # Needed for generator expressions to work.
    VERBATIM
  )
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
