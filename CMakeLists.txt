# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cmake_minimum_required(VERSION 3.20..3.27)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Setup the project.
project(tit VERSION 0.1 LANGUAGES CXX)

# Setup path to CMake scripts and include them.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(clangd)
include(codespell)
include(compiler)
include(tit_target)
include(tit_testing)

# Find the third-party libraries.
find_package(doctest CONFIG REQUIRED)
find_package(HDF5 CONFIG REQUIRED)
find_package(hwy CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)

# Setup output directories.
# TODO: This must be gone once we switch to installation.
set(_ROOT_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/output/TIT_ROOT")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${_ROOT_OUTPUT_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${_ROOT_OUTPUT_DIR}/bin")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Setup C++ globally.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

# A container library that stores all the properties.
# Other libraries and executables should link with this library.
add_library(tit_base INTERFACE)
add_library(tit::base ALIAS tit_base)

# Setup include directories.
target_include_directories(tit_base INTERFACE "source")

# Setup configurations.
foreach(CONFIG ${ALL_CONFIGS})
  string(TOUPPER ${CONFIG} CONFIG_UPPER)
  # Apply compilation options.
  target_compile_options(
    tit_base INTERFACE
    $<$<CONFIG:${CONFIG}>:${CXX_COMPILE_OPTIONS_${CONFIG_UPPER}}>)
  # Apply link options.
  target_link_options(
    tit_base INTERFACE
    $<$<CONFIG:${CONFIG}>:${CXX_LINK_OPTIONS_${CONFIG_UPPER}}>)
endforeach()

# Link with the dependent libraries.
target_link_libraries(
  tit_base INTERFACE
  hdf5::hdf5-static)

# Write "compile_flags.txt" based on the container library.
write_compile_flags(tit::base)

# Add the actual sources.
add_subdirectory("source")
add_subdirectory("tests")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Enable global static analysis.
check_spelling(${CMAKE_PROJECT_NAME} "${CMAKE_SOURCE_DIR}")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
