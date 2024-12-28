# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# TODO: Add a proper wrapper function for creating Python bindings.
nanobind_add_module(
  _data
  _data.cpp
  NB_STATIC STABLE_ABI
)

target_link_libraries(_data PUBLIC tit::data)

enable_clang_tidy(_data)

install(TARGETS _data LIBRARY DESTINATION "python/pytit/data")

install(
  FILES
    "__init__.py"
    "_data.pyi"
  DESTINATION
    "python/pytit/data"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
