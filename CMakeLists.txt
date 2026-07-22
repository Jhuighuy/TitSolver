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

# Export compile commands for the IDEs and analysis tools.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Some of the third-party libraries are not configured for the modern CMake.
# We have to drop the minimum required version to make them work.
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

# Find the third-party libraries.
find_package(assimp CONFIG REQUIRED)
find_package(cpptrace CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)
find_package(gcem CONFIG REQUIRED)
find_package(HighFive CONFIG REQUIRED)
find_package(hwy CONFIG REQUIRED)
find_package(mimalloc CONFIG REQUIRED)
find_package(MPI REQUIRED COMPONENTS CXX)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SymEngine CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(unofficial-node-addon-api CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)

# MPI installations include a launcher and a matching component/plugin tree;
# they are an external runtime rather than a few libraries that can be copied
# into TIT_ROOT. Preserve the configured MPI library directories in installed
# command-line tools so that the library and its components come from the same
# installation.
set(TIT_MPI_RUNTIME_DIRS)
foreach(MPI_LIBRARY IN LISTS MPI_CXX_LIBRARIES)
  if(IS_ABSOLUTE "${MPI_LIBRARY}")
    cmake_path(GET MPI_LIBRARY PARENT_PATH MPI_LIBRARY_DIR)
    list(APPEND TIT_MPI_RUNTIME_DIRS "${MPI_LIBRARY_DIR}")
  endif()
endforeach()
list(REMOVE_DUPLICATES TIT_MPI_RUNTIME_DIRS)

# SymEngine target does not properly link to its own dependencies.
target_link_libraries(symengine INTERFACE ${SYMENGINE_LIBRARIES})
add_library(symengine::symengine ALIAS symengine)

# Some packages are marked as "unofficial" by vcpkg, let's rename them.
add_library(node-addon-api::node-addon-api ALIAS unofficial::node-addon-api::node-addon-api)
add_library(sqlite3::sqlite3 ALIAS unofficial::sqlite3::sqlite3)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Include CMake modules.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(clang)
include(codespell)
include(git)
include(node)
include(testing)
include(tit_target)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Add the source directories.
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
    assimp::assimp
    cpptrace::cpptrace
    doctest::doctest
    gcem
    HighFive::HighFive
    hwy::hwy
    mimalloc-static
    MPI::MPI_CXX
    nlohmann_json::nlohmann_json
    node-addon-api::node-addon-api
    sqlite3::sqlite3
    symengine::symengine
    TBB::tbb
    zstd::libzstd
)

# Write "compile_flags.txt" based on the meta library.
write_compile_flags(all_flags)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Install licenses for vcpkg packages.
cmake_path(SET VCPKG_SHARE_DIR
  "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share"
)
if(EXISTS "${VCPKG_SHARE_DIR}")
  file(GLOB PACKAGE_DIRS LIST_DIRECTORIES true "${VCPKG_SHARE_DIR}/*")
  foreach(PKG_DIR ${PACKAGE_DIRS})
    get_filename_component(PKG_NAME "${PKG_DIR}" NAME)
    if(EXISTS "${PKG_DIR}/copyright")
      install(FILES "${PKG_DIR}/copyright" DESTINATION "licenses/${PKG_NAME}")
    endif()
  endforeach()
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Enable spell checking.
check_spelling()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
