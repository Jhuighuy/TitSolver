/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "tit/core/exception.hpp"
#include "tit/prop/tree.hpp"

namespace tit::prop {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto normalized_extension(const std::filesystem::path& path) -> std::string {
  auto ext = path.extension().string();
  std::ranges::transform(ext, ext.begin(), [](char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return ext;
}

auto read_text_file(const std::filesystem::path& path) -> std::string {
  const std::ifstream file{path};
  TIT_ENSURE(file.good(), "Failed to open property file '{}'.", path);
  std::ostringstream buffer{};
  buffer << file.rdbuf();
  TIT_ENSURE(file.good(), "Failed to read property file '{}'.", path);
  return std::move(buffer).str();
}

void write_text_file(const std::filesystem::path& path, std::string_view text) {
  std::ofstream file{path};
  TIT_ENSURE(file.good(), "Failed to open property file '{}'.", path);
  file << text;
  TIT_ENSURE(file.good(), "Failed to write property file '{}'.", path);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

auto tree_from_file(const std::filesystem::path& path) -> Tree {
  const auto ext = normalized_extension(path);
  if (ext == ".json") {
    return tree_from_json(read_text_file(path));
  }
  if (ext == ".yaml" || ext == ".yml") {
    return tree_from_yaml(read_text_file(path));
  }
  TIT_THROW("Unsupported property file extension '{}' for '{}'.", ext, path);
}

void tree_dump_file(const Tree& tree, const std::filesystem::path& path) {
  const auto ext = normalized_extension(path);
  if (ext == ".json") {
    write_text_file(path, tree_dump_json(tree));
    return;
  }
  if (ext == ".yaml" || ext == ".yml") {
    write_text_file(path, tree_dump_yaml(tree));
    return;
  }
  TIT_THROW("Unsupported property file extension '{}' for '{}'.", ext, path);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
