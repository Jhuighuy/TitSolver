# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    titsdk
  TYPE
    SHARED
  SOURCES
    "_utils.hpp"
    "lib.cpp"
    "lib.hpp"
    "titsdk.hpp"
    "ttdb.cpp"
    "ttdb.hpp"
  DEPENDS
    tit::data
  DESTINATION
    "lib/titsdk"
)

install(
  FILES
    "lib.hpp"
    "titsdk.hpp"
    "ttdb.hpp"
  DESTINATION
    "lib/titsdk"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

install(
  FILES
    "__init__.py"
    "lib.py"
    "ttdb.py"
  DESTINATION
    "lib/titsdk"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
