# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find the npm executable.
find_program(NPM_EXE NAMES "npm" REQUIRED)

# Find the npx executable.
find_program(NPX_EXE NAMES "npx" REQUIRED)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add a node package target.
#
function(add_tit_node_package)
  # Parse and check arguments.
  cmake_parse_arguments(TARGET "" "NAME" "DEPENDS" ${ARGN})
  set(TARGET "${TARGET_NAME}")

  # Find "package.json".
  set(TARGET_PACKAGE_JSON "${CMAKE_CURRENT_SOURCE_DIR}/package.json")
  if(NOT EXISTS ${TARGET_PACKAGE_JSON})
    message(FATAL_ERROR "'package.json' file does not exist!")
  endif()

  # Find all the top-level source files (non-recursive).
  set(TARGET_SOURCES)
  set(EXTS
    ".html" ".css"
    ".svg" ".icns"
    ".js" ".jsx" ".mjs" ".json"
    ".ts" ".tsx" ".mts"
  )
  foreach(EXT ${EXTS})
    file(GLOB EXT_SOURCES
      LIST_DIRECTORIES false
      "${CMAKE_CURRENT_SOURCE_DIR}/*${EXT}"
    )
    list(APPEND TARGET_SOURCES ${EXT_SOURCES})
  endforeach()

  # Find all the source files in child directories.
  set(EXCLUDED_DIRS ".vite" "dist" "coverage" "node_modules")
  file(GLOB TARGET_CHILDREN
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    LIST_DIRECTORIES true
    "${CMAKE_CURRENT_SOURCE_DIR}/*"
  )
  foreach(CHILD ${TARGET_CHILDREN})
    if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${CHILD}")
      continue()
    endif()
    if(CHILD IN_LIST EXCLUDED_DIRS)
      continue()
    endif()
    foreach(EXT ${EXTS})
      file(GLOB_RECURSE EXT_CHILD_SOURCES
        LIST_DIRECTORIES false
        "${CMAKE_CURRENT_SOURCE_DIR}/${CHILD}/*${EXT}"
      )
      list(APPEND TARGET_SOURCES ${EXT_CHILD_SOURCES})
    endforeach()
  endforeach()

  # Run `npm install`.
  set(STAMP "${CMAKE_CURRENT_SOURCE_DIR}/node_modules/.package-lock.json")
  add_custom_command(
    COMMENT "Installing dependencies for Node package ${TARGET}"
    COMMAND "${CHRONIC_EXE}" "${NPM_EXE}" install
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${TARGET_PACKAGE_JSON}" ${TARGET_DEPENDS}
    OUTPUT "${STAMP}"
  )
  add_custom_target("${TARGET}_install" ALL DEPENDS "${STAMP}")

  # Run `npx generate-license-file` to create a file with full license text for
  # all the third-party dependencies.
  set(LICENSES "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_third_party_licenses.txt")
  add_custom_command(
    COMMENT "Generating third-party license file for Node package ${TARGET}"
    COMMAND
      "${CHRONIC_EXE}"
        "${NPX_EXE}" --yes
          generate-license-file
            --input "${TARGET_PACKAGE_JSON}"
            --output "${LICENSES}"
            --overwrite
    COMMAND sed -i -e '/generate-license-file/d' "${LICENSES}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${TARGET_PACKAGE_JSON}" "${TARGET}_install"
    OUTPUT "${LICENSES}"
  )
  add_custom_target("${TARGET}_licenses" ALL DEPENDS "${LICENSES}")
  install(FILES "${LICENSES}" DESTINATION "licenses")

  # Run `npm run build`.
  set(STAMP "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.stamp")
  add_custom_command(
    COMMENT "Building Node package ${TARGET}"
    COMMAND "${CHRONIC_EXE}" "${NPM_EXE}" run build
    COMMAND "${CMAKE_COMMAND}" -E touch "${STAMP}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS ${TARGET_SOURCES} "${TARGET}_install" "${TARGET}_licenses"
    OUTPUT "${STAMP}"
  )
  add_custom_target("${TARGET}" ALL DEPENDS "${STAMP}")

  # Run the linters.
  if(NOT SKIP_ANALYSIS)
    set(STAMP "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_lint.stamp")
    add_custom_command(
      COMMENT "Linting Node package ${TARGET}"
      COMMAND "${CHRONIC_EXE}" "${NPM_EXE}" run lint
      COMMAND "${CMAKE_COMMAND}" -E touch "${STAMP}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      DEPENDS ${TARGET_SOURCES} "${TARGET}"
      OUTPUT "${STAMP}"
    )
    add_custom_target("${TARGET}_lint" ALL DEPENDS "${STAMP}")
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
