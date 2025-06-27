#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Format all indexed files.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Print usage information if requested.
if [[ "$*" == *"-h"* || "$*" == *"--help"* ]]; then
  echo "Usage: $(basename "$0") [path ...]"
  echo "Formats all indexed files in the git index."
  exit 1
fi

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Run clang-format on all the C/C++ files.
echo "Formatting C/C++ files..."
git ls-files "$@" |
  grep -E "(.*/_cxx/.*)|(\.(c|h)(pp)?$)" |
  xargs clang-format -i

# Run Prettier on all the web files.
echo "Formatting web files..."
git ls-files "$@" |
  grep -E "\.(js|ts|jsx|tsx|css|html|json)$" |
  xargs prettier --write

# Run YAPF on all the Python files.
echo "Formatting Python files..."
git ls-files "$@" |
  grep -E "\.py$" |
  xargs yapf -i

echo "All done!"

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
