# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "Tit"
copyright = "2023, Dev"
author = "Dev"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ["breathe"]

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "alabaster"

# Breathe Configuration
breathe_projects = {
    "tit_core": "/Users/jhuighuy/TitSolver/output/cmake_output/source/tit/core/doxygen_xml",
    "tit_par": "/Users/jhuighuy/TitSolver/output/cmake_output/source/tit/par/doxygen_xml",
}
breathe_default_project = "tit_core"
html_theme = "sphinx_book_theme"
breathe_show_include = False
