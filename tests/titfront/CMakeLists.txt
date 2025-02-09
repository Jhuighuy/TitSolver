# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
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
