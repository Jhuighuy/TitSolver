set(ROOT_OUTPUT_DIR "${ROOT_OUTPUT_DIR}/docs/")
set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${ROOT_OUTPUT_DIR})

add_custom_target(Sphinx ALL
                  COMMAND
                  ${SPHINX_BUILD_EXE} -b html
                  ${SPHINX_SOURCE} ${SPHINX_BUILD}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Generating documentation with Sphinx")
