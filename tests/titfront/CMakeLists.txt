# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

get_target_property(FRONT_SOURCE_DIR titfront SOURCE_DIR)

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  set(RUN_SCRIPT "coverage")
else()
  set(RUN_SCRIPT "test")
endif()

add_tit_test(
  NAME "titfront"
  COMMAND "${BASH_EXE}" -c "cd ${FRONT_SOURCE_DIR} && pnpm run ${RUN_SCRIPT}"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
