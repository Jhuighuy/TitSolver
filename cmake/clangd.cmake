# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

include_guard()
include(clang)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

## Write the `compile_flags.txt` for the clangd language server.
function(write_compile_flags TARGET_OR_ALIAS)
  # Get the original target name if it is an alias.
  get_target_property(TARGET ${TARGET_OR_ALIAS} ALIASED_TARGET)
  if(NOT TARGET ${TARGET})
    set(TARGET ${TARGET_OR_ALIAS})
  endif()
  # Setup "compilation" arguments for mock "clang" call.
  set(CLANG_COMPILE_ARGS)
  ## Get generated compile options from target.
  get_generated_compile_options(${TARGET} CLANG_COMPILE_ARGS)
  ## Append some extra options.
  list(
    APPEND
    CLANG_COMPILE_ARGS
    # Enable the same set of warnings we would use for normal clang.
    ${CLANG_WARNINGS}
    # Enable C++23 (`c++2b` and not `c++23` for clang-16).
    -std=c++2b)
  if(APPLE)
    ## Force use libstdc++ since libc++ misses some stuff.
    clang_force_use_libstdcpp(CLANG_COMPILE_ARGS)
  endif()
  # Write the compile flags to a file.
  add_custom_target(
    "${TARGET}_clangd_compile_flags"
    ## Run each time during the build process.
    ALL
    ## Write the compile flags to a file, each on a new line.
    COMMAND
      "${CMAKE_COMMAND}" -E echo ${CLANG_COMPILE_ARGS} |
      xargs -n 1 > "${CMAKE_CURRENT_SOURCE_DIR}/compile_flags.txt"
    ## Add message.
    COMMENT "Writing compile_flags.txt"
    ## This is needed for generator expressions to work.
    COMMAND_EXPAND_LISTS
    VERBATIM)
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
