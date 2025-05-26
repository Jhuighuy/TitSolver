/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp> // NOLINT(misc-include-cleaner)
#include <nlohmann/json_fwd.hpp>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/prop/tree.hpp"

namespace tit::prop {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using JSON = nlohmann::ordered_json;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tree_to_json(const Tree& tree) -> JSON {
  if (tree.is_null()) return nullptr;
  if (tree.is_bool()) return tree.as_bool();
  if (tree.is_int()) return tree.as_int();
  if (tree.is_real()) return tree.as_real();
  if (tree.is_string()) return tree.as_string();
  if (tree.is_array()) {
    auto json = JSON::array();
    for (size_t i = 0; i < tree.size(); ++i) {
      json.push_back(tree_to_json(tree.get(i)));
    }
    return json;
  }
  if (tree.is_map()) {
    auto json = JSON::object();
    for (const auto& key : tree.keys()) {
      json[key] = tree_to_json(tree.get(key));
    }
    return json;
  }
  std::unreachable();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tree_from_json(const JSON& json) -> Tree {
  if (json.is_null()) return Tree{};
  if (json.is_boolean()) return Tree{json.get<bool>()};
  if (json.is_number_integer()) return Tree{json.get<int64_t>()};
  if (json.is_number()) return Tree{json.get<float64_t>()};
  if (json.is_string()) return Tree{json.get<std::string>()};
  if (json.is_array()) {
    Tree::Array array;
    for (const auto& item : json) {
      array.push_back(tree_from_json(item));
    }
    return Tree{std::move(array)};
  }
  if (json.is_object()) {
    Tree::Map map;
    for (const auto& [key, val] : json.items()) {
      map[key] = tree_from_json(val);
    }
    return Tree{std::move(map)};
  }
  std::unreachable();
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tree_dump_json(const Tree& tree) -> std::string {
  return tree_to_json(tree).dump(2);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tree_from_json(std::string_view string) -> Tree {
  JSON json;
  try {
    json = JSON::parse(string);
  } catch (const JSON::exception& exc) {
    TIT_THROW("Failed to parse JSON string '{}': {}", string, exc.what());
  }
  return tree_from_json(json);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
