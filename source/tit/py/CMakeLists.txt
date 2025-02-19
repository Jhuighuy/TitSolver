# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    py
  SOURCES
    "_python.hpp"
    "capsule.hpp"
    "capsule.cpp"
    "class.cpp"
    "class.hpp"
    "error.cpp"
    "error.hpp"
    "func.cpp"
    "func.hpp"
    "gil.cpp"
    "gil.hpp"
    "iterator.cpp"
    "iterator.hpp"
    "mapping.cpp"
    "mapping.hpp"
    "module.cpp"
    "module.hpp"
    "number.cpp"
    "number.hpp"
    "numpy.cpp"
    "numpy.hpp"
    "object.cpp"
    "object.hpp"
    "sequence.cpp"
    "sequence.hpp"
    "typing.hpp"
  DEPENDS
    tit::core
    # header-only part of tit::data
)

# Since we are not linking to Python here, some manual configuration is needed.
target_include_directories(
  tit_py
  PUBLIC
    "${Python3_INCLUDE_DIRS}"
    "${Python3_NumPy_INCLUDE_DIRS}"
)
target_compile_options(
  tit_py
  PUBLIC
    "-Wno-cast-qual"
    "-Wno-old-style-cast"
    "-Wno-redundant-decls"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    py_embed
  SOURCES
    "interpreter.cpp"
    "interpreter.hpp"
  DEPENDS
    tit::core
    tit::py
    Python3::Python
    Python3::NumPy
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    py_module
  DEPENDS
    tit::py
    Python3::Module
    Python3::NumPy
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    py_tests
  SOURCES
    "capsule.test.cpp"
    "class.test.cpp"
    "error.test.cpp"
    "func.test.cpp"
    "gil.test.cpp"
    "interpreter.test.cpp"
    "interpreter.testing.cpp"
    "interpreter.testing.hpp"
    "iterator.test.cpp"
    "mapping.test.cpp"
    "module.test.cpp"
    "number.test.cpp"
    "numpy.test.cpp"
    "object.test.cpp"
    "sequence.test.cpp"
    "typing.test.cpp"
  DEPENDS
    tit::py_embed
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
