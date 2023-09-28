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

# Set output directories.
# (Set globally, since these properties are not inherited by targets.)
set(OUTPUT_DIR ../output/TIT_ROOT)
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${OUTPUT_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${OUTPUT_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${OUTPUT_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${OUTPUT_DIR}/lib)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Set path to CMake scripts.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Include the functionality.
include(IWYU)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# A container library that stores all the properties.
add_library(tit_iface INTERFACE)
add_library(tit::iface ALIAS tit_iface)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Check C/C++ compiler.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CXX_COMPILER_IS_GNU YES)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.1")
    message(FATAL_ERROR "GCC version 13.1 or more recent is required!")
  endif()
else()
  message(FATAL_ERROR "Unsupported C++ compiler!")
endif()

# Require C++23 and disable extensions.
# (Set globally, since these properties are not inherited by targets.)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

set(CXX_DEFINITIONS "")
set(CXX_COMPILE_OPTIONS "")
set(CXX_LINK_OPTIONS "")

# Set the warnings and diagnostics options.
list(APPEND CXX_COMPILE_OPTIONS
     #-Werror -Wall -Wextra -Wpedantic -Wno-unused-function -Wno-psabi
     )

# Set the optimization options.
list(APPEND CXX_COMPILE_OPTIONS
     -g -Ofast -march=native -finline-limit=10000000 -ffp-contract=fast)

# Apply the options.
target_compile_definitions(tit_iface INTERFACE ${CXX_DEFINITIONS})
target_compile_options(tit_iface INTERFACE ${CXX_COMPILE_OPTIONS})
target_link_options(tit_iface INTERFACE ${CXX_LINK_OPTIONS})

# Set include directory.
target_include_directories(tit_iface INTERFACE ${CMAKE_SOURCE_DIR})

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Find {fmt}.
find_package(fmt CONFIG REQUIRED QUIET)
message(STATUS "| fmt enabled.")
target_link_libraries(tit_iface INTERFACE fmt::fmt-header-only)

# Find spdlog.
find_package(spdlog CONFIG REQUIRED QUIET)
message(STATUS "| spdlog enabled.")
target_link_libraries(tit_iface INTERFACE spdlog::spdlog_header_only)

# Find Thread Bulding Blocks.
find_package(TBB QUIET)
if(TBB_FOUND)
  set(TIT_HAS_TBB 1)
  message(STATUS "| TBB enabled.")
  target_link_libraries(tit_iface INTERFACE TBB::tbb)
endif()

# Find OpenMP.
find_package(OpenMP QUIET)
if(OpenMP_FOUND)
  set(TIT_HAS_OPENMP 1)
  message(STATUS "| OpenMP enabled.")
  target_link_libraries(tit_iface INTERFACE OpenMP::OpenMP_CXX)
endif()

# Find HDF5.
find_package(HDF5 QUIET)
if(HDF5_FOUND)
  set(TIT_HAS_HDF5 1)
  target_link_libraries(tit_iface INTERFACE hdf5::hdf5)
  message(STATUS "| HDF5 enabled.")
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

add_subdirectory(tit)
add_subdirectory(titesph)
add_subdirectory(titgui)
add_subdirectory(titwcsph)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
