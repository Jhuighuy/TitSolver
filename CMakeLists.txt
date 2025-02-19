# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cmake_minimum_required(VERSION 3.20..3.27)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Setup the project.
project(tit VERSION 0.1 LANGUAGES CXX)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Setup path to CMake modules and include them.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(clang)
include(codespell)
include(python)
include(tit_target)
include(tit_testing)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Install the third-party Python packages.
install_python(
  DESTINATION
    python
  PACKAGES
    "coverage[toml]"
    "numpy"
)

# Find the third-party libraries.
find_package(
  Boost
  CONFIG REQUIRED
  COMPONENTS core container stacktrace_addr2line
)
find_package(Crow CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)
find_package(gcem CONFIG REQUIRED)
find_package(hwy CONFIG REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
find_package(TBB CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Add the actual sources.
add_subdirectory("manual")
add_subdirectory("source")
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
    Boost::core
    Boost::stacktrace_addr2line
    Crow::Crow
    doctest::doctest
    gcem
    hwy::hwy
    metis
    Python3::NumPy
    Python3::Python
    TBB::tbb
    TBB::tbbmalloc
    unofficial::sqlite3::sqlite3
    zstd::libzstd
)

# Write "compile_flags.txt" based on the meta library.
write_compile_flags(tit::all_flags)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Enable spell checking.
check_spelling()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
