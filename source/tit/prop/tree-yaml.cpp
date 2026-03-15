/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

#include <yaml-cpp/exceptions.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/node/type.h>
#include <yaml-cpp/yaml.h> // IWYU pragma: keep

#include "tit/core/exception.hpp"
#include "tit/prop/tree.hpp"

namespace tit::prop {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tree_from_yaml(const YAML::Node& yaml) -> Tree {
  switch (yaml.Type()) {
    case YAML::NodeType::Null:
    case YAML::NodeType::Undefined: {
      return Tree{};
    }

    case YAML::NodeType::Scalar: {
      return Tree{yaml.Scalar()};
    }

    case YAML::NodeType::Sequence: {
      Tree::Array array;
      for (const auto& item : yaml) {
        array.push_back(tree_from_yaml(item));
      }
      return Tree{std::move(array)};
    }

    case YAML::NodeType::Map: {
      Tree::Map map;
      for (const auto& kv : yaml) {
        // Note: For some reason, `kv` is not decomposable.
        const auto& key = kv.first;
        const auto& val = kv.second;
        TIT_ENSURE(key.IsScalar(), "YAML map key must be a scalar.");
        map[key.as<std::string>()] = tree_from_yaml(val);
      }
      return Tree{std::move(map)};
    }
  }
  std::unreachable();
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tree_from_yaml(const std::filesystem::path& path) -> Tree {
  std::ifstream file{path};
  TIT_ENSURE(file.is_open(), "Cannot open YAML file '{}'.", path.string());

  YAML::Node node;
  try {
    node = YAML::Load(file);
  } catch (const YAML::Exception& e) {
    TIT_THROW("Failed to parse YAML config '{}': {}", path.string(), e.what());
  }
  TIT_ENSURE(node.IsMap(),
             "File '{}' must be a YAML mapping at the top level.",
             path.string());

  return tree_from_yaml(node);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
