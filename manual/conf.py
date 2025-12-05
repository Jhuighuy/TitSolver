# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# pylint: disable=invalid-name,redefined-builtin,missing-module-docstring
import datetime
import json
import os
import sys
from unittest import mock

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# Python configuration.
#

MANUAL_ROOT_DIR = os.path.dirname(__file__)
PROJECT_ROOT_DIR = os.path.abspath(os.path.join(MANUAL_ROOT_DIR, ".."))
SOURCE_DIR = os.path.abspath(os.path.join(PROJECT_ROOT_DIR, "source"))
sys.path.insert(0, SOURCE_DIR)
sys.modules["ctypes"] = mock.Mock()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# Project information.
#

project = "BlueTit Solver"
copyright = f"{datetime.date.today().year}"
author = "Oleg Butakov"
with open(os.path.join(PROJECT_ROOT_DIR, "vcpkg.json"), encoding="utf-8") as f:
    release = json.load(f)["version-string"]

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# General configuration.
#

extensions: list[str] = []
nitpick_ignore_regex: list[tuple[str, str]] = []
exclude_patterns = ["Thumbs.db", ".DS_Store", "README.md"]
templates_path = ["_templates"]
numfig = True
numfig_format = {
    "figure": "Figure %s.",
    "table": "Table %s.",
    "code-block": "Code %s.",
}
rst_prolog = r"""
.. |product| replace:: :math:`\mathsf{BlueTit\text{ }Solver}`
"""

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# Options for HTML output.
#

html_short_title = "BlueTit Solver"
html_copy_source = False
html_permalinks = False
html_theme = "basic"
html_static_path = ["_static"]

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
