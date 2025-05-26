/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/prop/tree.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tree::Tree(bool val) : data_{val} {}
Tree::Tree(int64_t val) : data_{val} {}
Tree::Tree(float64_t val) : data_{val} {}
Tree::Tree(std::string val) : data_{std::move(val)} {}
Tree::Tree(Array val) : data_{std::move(val)} {}
Tree::Tree(Map val) : data_{std::move(val)} {}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Tree::is_null() const noexcept -> bool {
  return std::holds_alternative<std::monostate>(data_);
}

auto Tree::is_bool() const noexcept -> bool {
  return std::holds_alternative<bool>(data_);
}

auto Tree::is_int() const noexcept -> bool {
  return std::holds_alternative<int64_t>(data_);
}

auto Tree::is_real() const noexcept -> bool {
  return std::holds_alternative<float64_t>(data_);
}

auto Tree::is_string() const noexcept -> bool {
  return std::holds_alternative<std::string>(data_);
}

auto Tree::is_array() const noexcept -> bool {
  return std::holds_alternative<Array>(data_);
}

auto Tree::is_map() const noexcept -> bool {
  return std::holds_alternative<Map>(data_);
}

auto Tree::type_name() const noexcept -> std::string_view {
  if (is_null()) return "null";
  if (is_bool()) return "boolean";
  if (is_int()) return "integer";
  if (is_real()) return "number";
  if (is_string()) return "string";
  if (is_array()) return "array";
  if (is_map()) return "map";
  std::unreachable();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Tree::as_bool() const -> bool {
  TIT_ENSURE(is_bool(), "Property is not a boolean.");
  return std::get<bool>(data_);
}

auto Tree::as_int() const -> int64_t {
  TIT_ENSURE(is_int(), "Property is not an integer.");
  return std::get<int64_t>(data_);
}

auto Tree::as_real() const -> float64_t {
  TIT_ENSURE(is_real(), "Property is not a number.");
  return std::get<float64_t>(data_);
}

auto Tree::as_string() const -> std::string_view {
  TIT_ENSURE(is_string(), "Property is not a string.");
  return std::get<std::string>(data_);
}

auto Tree::as_array() -> Array& {
  TIT_ENSURE(is_array(), "Property is not an array.");
  return std::get<Array>(data_);
}
auto Tree::as_array() const -> const Array& {
  TIT_ENSURE(is_array(), "Property is not an array.");
  return std::get<Array>(data_);
}

auto Tree::as_map() -> Map& {
  TIT_ENSURE(is_map(), "Property is not a map.");
  return std::get<Map>(data_);
}
auto Tree::as_map() const -> const Map& {
  TIT_ENSURE(is_map(), "Property is not a map.");
  return std::get<Map>(data_);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Tree::size() const -> size_t {
  TIT_ENSURE(is_array() || is_map(), "Property is not an array or map.");
  return is_array() ? as_array().size() : as_map().size();
}

auto Tree::get(size_t index) -> Tree& {
  auto& array = as_array();
  TIT_ENSURE(index < array.size(),
             "Array index {} is out of range (size {}).",
             index,
             array.size());
  return array[index];
}
auto Tree::get(size_t index) const -> const Tree& {
  const auto& array = as_array();
  TIT_ENSURE(index < array.size(),
             "Array index {} is out of range (size {}).",
             index,
             array.size());
  return array[index];
}

auto Tree::get(std::string_view key) -> Tree& {
  auto& map = as_map();
  const auto [iter, _] = map.try_emplace(std::string{key});
  return iter->second;
}
auto Tree::get(std::string_view key) const -> const Tree& {
  const auto& map = as_map();
  const auto iter = map.find(key);
  TIT_ENSURE(iter != map.end(), "Map key '{}' not found.", key);
  return iter->second;
}

auto Tree::has(std::string_view key) const -> bool {
  return as_map().contains(key);
}

auto Tree::keys() const -> std::vector<std::string> {
  return as_map() | std::views::keys | std::ranges::to<std::vector>();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Tree::merge(Tree other) {
  if (other.is_null()) return;
  if (is_null()) {
    *this = std::move(other);
    return;
  }
  if (is_map() && other.is_map()) {
    for (auto& [key, ptr] : other.as_map()) {
      get(key).merge(std::move(ptr));
    }
    return;
  }
  *this = std::move(other);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Tree::set(bool val) {
  data_ = val;
}

void Tree::set(int64_t val) {
  data_ = val;
}

void Tree::set(float64_t val) {
  data_ = val;
}

void Tree::set(std::string val) {
  data_ = std::move(val);
}

void Tree::set(Array val) {
  data_ = std::move(val);
}

void Tree::set(Map val) {
  data_ = std::move(val);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
