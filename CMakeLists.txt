# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

cmake_minimum_required(VERSION 3.20..3.27)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Setup the project.
project(Tit VERSION 0.1 LANGUAGES CXX)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Setup root source and output directories.
set(ROOT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/source")
set(ROOT_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/output/TIT_ROOT")

# Setup path to CMake scripts and include them.
list(APPEND CMAKE_MODULE_PATH "${ROOT_SOURCE_DIR}/cmake")
include(clang_tidy)
include(config)
include(iwyu)

# Setup output directories. This must be gone once we switch to installation.
set(CMAKE_PDB_OUTPUT_DIRECTORY "${ROOT_OUTPUT_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ROOT_OUTPUT_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ROOT_OUTPUT_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ROOT_OUTPUT_DIR}/lib")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# A container library that stores all the properties. Other libraries and
# executables should link with this library.
add_library(tit_base INTERFACE)
add_library(tit::base ALIAS tit_base)

# Setup C++: require C++23 and disable extensions.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

# Setup include directories.
target_include_directories(tit_base INTERFACE ${ROOT_SOURCE_DIR})

# Setup configurations.
foreach(CONFIG ${ALL_CONFIGS})
  string(TOUPPER ${CONFIG} CONFIG_UPPER)
  # Apply compilation options.
  target_compile_options(
    tit_base
    INTERFACE
    $<$<CONFIG:${CONFIG}>:${CXX_COMPILE_OPTIONS_${CONFIG_UPPER}}>)
  # Apply link options.
  target_link_options(
    tit_base
    INTERFACE
    $<$<CONFIG:${CONFIG}>:${CXX_LINK_OPTIONS_${CONFIG_UPPER}}>)
endforeach()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Find {fmt}.
find_package(fmt CONFIG REQUIRED QUIET)
target_link_libraries(tit_base INTERFACE fmt::fmt)

# Find spdlog.
find_package(spdlog CONFIG REQUIRED QUIET)
target_link_libraries(tit_base INTERFACE spdlog::spdlog)

## Find Thread Bulding Blocks.
#find_package(TBB CONFIG REQUIRED QUIET)
#target_link_libraries(tit_base INTERFACE TBB::tbb TBB::tbbmalloc)

# Find OpenMP.
find_package(OpenMP REQUIRED QUIET)
target_link_libraries(tit_base INTERFACE OpenMP::OpenMP_CXX)

# Find HDF5.
find_package(HDF5 CONFIG REQUIRED QUIET)
target_link_libraries(tit_base INTERFACE hdf5::hdf5-static)

# Find Qt.
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()
# Do not link Qt to the base library!

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Add source subdirectory.
add_subdirectory("source")

# Add tests subdirectory.
add_subdirectory("tests")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
