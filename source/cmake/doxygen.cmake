# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

include_guard()
include(utils)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Find Doxygen (at least 1.9.7).
find_program_with_version(
  DOXYGEN_EXE
  NAMES doxygen
  MIN_VERSION "1.9.7")

if(DOXYGEN_EXE)
  # Find Doxygen configuration template file.
  find_file(
    DOXYGEN_CONFIG_TEMPLATE_PATH
    NAME "Doxyfile.in"
    PATHS ${ROOT_SOURCE_DIR}/cmake
    REQUIRED)

  # Find xsltproc.
  find_program(XSLTPROC_EXE NAMES xsltproc REQUIRED)
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

## Generate documentation with Doxygen.
function(generate_documentation TARGET_OR_ALIAS)
  # Exit early in case sufficient Doxygen was not found.
  if(NOT DOXYGEN_EXE)
    return()
  endif()
  # Get the original target name if it is an alias.
  get_target_property(TARGET ${TARGET_OR_ALIAS} ALIASED_TARGET)
  if(NOT TARGET ${TARGET})
    set(TARGET ${TARGET_OR_ALIAS})
  endif()
  # Prepare Doxygen configuration file.
  ## Get a list of target's sources.
  get_target_property(TARGET_SOURCE_DIR ${TARGET} SOURCE_DIR)
  get_target_property(TARGET_SOURCES ${TARGET} SOURCES)
  list(TRANSFORM TARGET_SOURCES PREPEND "${TARGET_SOURCE_DIR}/")
  list(JOIN TARGET_SOURCES " " DOXYGEN_INPUT)
  ## Prepare directory to store the intermidiate (uncombined) output.
  set(DOXYGEN_XML_OUTPUT_PATH "doxygen_xml/")
  ## Use CMake's configuration capabilities to generate the Doxyfile for us.
  set(DOXYGEN_CONFIG_PATH "${TARGET}.doxyfile")
  configure_file(
    ${DOXYGEN_CONFIG_TEMPLATE_PATH}
    ${DOXYGEN_CONFIG_PATH}
    @ONLY)
  # Run Doxygen.
  set(DOXYGEN_XML_PATH "docs.xml")
  add_custom_command(
    OUTPUT ${DOXYGEN_XML_PATH}
    ## Run Doxygen. It will generate a folder with many `.xml` files.
    COMMAND
      "${DOXYGEN_EXE}" "${DOXYGEN_CONFIG_PATH}"
    ## Combine the output using xsltproc.
    COMMAND
      "${XSLTPROC_EXE}"
      "${DOXYGEN_XML_OUTPUT_PATH}/combine.xslt"
      "${DOXYGEN_XML_OUTPUT_PATH}/index.xml" > "${DOXYGEN_XML_PATH}"
    ## Documentation depends on both configuration file (that modifies once
    ## list of sources change) and on all the target's source files.
    DEPENDS ${DOXYGEN_CONFIG_PATH} ${TARGET_SOURCES}
    COMMENT "Generating documetation for ${TARGET}")
  # Create a custom target that should "build" once all generation succeed.
  add_custom_target(
    "${TARGET}_doxygen"
    ALL
    DEPENDS ${TARGET} ${DOXYGEN_CONFIG_PATH} ${DOXYGEN_XML_PATH})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
