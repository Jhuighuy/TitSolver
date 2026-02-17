# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cmake_minimum_required(VERSION 3.20..3.27)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Read the version from vcpkg manifest and setup the project.
file(READ "${CMAKE_SOURCE_DIR}/vcpkg.json" VCPKG_MANIFEST)
string(JSON TIT_VERSION GET "${VCPKG_MANIFEST}" "version-string")
project(tit VERSION "${TIT_VERSION}" LANGUAGES CXX)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Setup path to CMake modules and include them.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(clang)
include(codespell)
include(git)
include(pnpm)
include(sphinx)
include(testing)
include(tit_target)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Find the third-party libraries.
find_package(Boost CONFIG REQUIRED COMPONENTS container stacktrace_addr2line)
find_package(Clipper2 CONFIG REQUIRED)
find_package(Crow CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)
find_package(gcem CONFIG REQUIRED)
find_package(HighFive CONFIG REQUIRED)
find_package(hwy CONFIG REQUIRED)
find_package(mimalloc CONFIG REQUIRED)
find_package(miniz CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Add the actual sources.
add_subdirectory("source")
add_subdirectory("manual")
add_subdirectory("tests")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Create a meta target we would use to get all the compile flags.
# This target must depend on all the third-party libraries, and therefore has
# all the needed compile flags: include directories, compile definitions, etc.
add_tit_library(
  NAME
    all_flags
  DEPENDS
    Boost::container
    Boost::stacktrace_addr2line
    Clipper2::Clipper2
    Crow::Crow
    doctest::doctest
    gcem
    HighFive::HighFive
    hwy::hwy
    mimalloc-static
    miniz::miniz
    nlohmann_json::nlohmann_json
    TBB::tbb
    tinyxml2::tinyxml2
    unofficial::sqlite3::sqlite3
    zstd::libzstd
)

# Write "compile_flags.txt" based on the meta library.
write_compile_flags(tit_all_flags)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Enable spell checking.
check_spelling()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
