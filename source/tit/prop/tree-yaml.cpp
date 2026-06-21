/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include <yaml-cpp/emittermanip.h>
#include <yaml-cpp/yaml.h>

#include "tit/core/exception.hpp"
#include "tit/core/float.hpp"
#include "tit/core/str.hpp"
#include "tit/prop/tree.hpp"

namespace tit::prop {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tree_from_yaml(const YAML::Node& node) -> Tree {
  if (!node || node.IsNull()) return Tree{};
  if (node.IsScalar()) {
    const auto& scalar = node.Scalar();
    if (str_nocase_equal(scalar, "true")) return Tree{true};
    if (str_nocase_equal(scalar, "false")) return Tree{false};
    if (const auto value = str_to<std::int64_t>(scalar)) return Tree{*value};
    if (const auto value = str_to<float64_t>(scalar)) return Tree{*value};
    return Tree{scalar};
  }
  if (node.IsSequence()) {
    Tree::Array array;
    for (const auto& item : node) array.push_back(tree_from_yaml(item));
    return Tree{std::move(array)};
  }
  if (node.IsMap()) {
    Tree::Map map;
    for (const auto& item : node) {
      const auto key = item.first.as<std::string>();
      map[key] = tree_from_yaml(item.second);
    }
    return Tree{std::move(map)};
  }
  TIT_THROW("Unsupported YAML node.");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void tree_to_yaml(YAML::Emitter& emitter, const Tree& tree) {
  if (tree.is_null()) {
    emitter << std::string{"null"};
  } else if (tree.is_bool()) {
    emitter << tree.as_bool();
  } else if (tree.is_int()) {
    emitter << tree.as_int();
  } else if (tree.is_real()) {
    emitter << tree.as_real();
  } else if (tree.is_string()) {
    emitter << std::string{tree.as_string()};
  } else if (tree.is_array()) {
    emitter << YAML::BeginSeq;
    for (std::size_t i = 0; i < tree.size(); ++i) {
      tree_to_yaml(emitter, tree.get(i));
    }
    emitter << YAML::EndSeq;
  } else if (tree.is_map()) {
    emitter << YAML::BeginMap;
    for (const auto& key : tree.keys()) {
      emitter << YAML::Key << key;
      emitter << YAML::Value;
      tree_to_yaml(emitter, tree.get(key));
    }
    emitter << YAML::EndMap;
  } else {
    std::unreachable();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

auto tree_dump_yaml(const Tree& tree) -> std::string {
  YAML::Emitter emitter;
  tree_to_yaml(emitter, tree);
  TIT_ENSURE(emitter.good(),
             "Failed to dump YAML: {}.",
             emitter.GetLastError());
  return emitter.c_str();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tree_from_yaml(std::string_view string) -> Tree {
  try {
    return tree_from_yaml(YAML::Load(std::string{string}));
  } catch (const YAML::Exception& exc) {
    TIT_THROW("Failed to parse YAML: {}", exc.what());
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
