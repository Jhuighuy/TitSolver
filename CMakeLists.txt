# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

cmake_minimum_required(VERSION 3.20..3.26)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Setup the project.
project(Tit VERSION 0.1 LANGUAGES CXX)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Setup root output directory.
set(ROOT_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/output/TIT_ROOT)

# Setup output directories.
# (Set globally, since these properties are not inherited by targets.)
set(CMAKE_PDB_OUTPUT_DIRECTORY ${ROOT_OUTPUT_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_OUTPUT_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_OUTPUT_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ROOT_OUTPUT_DIR}/lib)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Set root source directory.
set(ROOT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

# Set path to CMake scripts.
list(APPEND CMAKE_MODULE_PATH ${ROOT_SOURCE_DIR}/cmake)

# Include the functionality.
include(clang_tidy)
include(config)
include(iwyu)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# A container library that stores all the properties.
add_library(tit_base INTERFACE)
add_library(tit::base ALIAS tit_base)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Require C++23 and disable extensions.
# (Set globally, since these properties are not inherited by targets.)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

# Apply the options.
target_compile_options(tit_base INTERFACE ${CXX_COMPILE_OPTIONS_RELEASE})
target_link_options(tit_base INTERFACE ${CXX_LINK_OPTIONS_RELEASE})

# Set include directory.
target_include_directories(tit_base INTERFACE ${ROOT_SOURCE_DIR})

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Find {fmt}.
find_package(fmt CONFIG REQUIRED QUIET)
target_link_libraries(tit_base INTERFACE fmt::fmt-header-only)

# Find spdlog.
find_package(spdlog CONFIG REQUIRED QUIET)
target_link_libraries(tit_base INTERFACE spdlog::spdlog_header_only)

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
add_subdirectory(${ROOT_SOURCE_DIR})

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
