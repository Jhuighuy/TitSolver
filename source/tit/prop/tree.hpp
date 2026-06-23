/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "tit/core/float.hpp"
#include "tit/core/str.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// A node in the parsed property tree.
class Tree final {
public:

  /// Array of property trees.
  using Array = std::vector<Tree>;

  /// Map of property trees.
  using Map = StrMap<Tree>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a null node.
  Tree() = default;

  /// Construct a boolean node.
  explicit Tree(bool val);

  /// Construct an integer node.
  /// @{
  explicit Tree(std::int64_t val);
  explicit Tree(std::integral auto val)
      : Tree(static_cast<std::int64_t>(val)) {}
  /// @}

  /// Construct a real node.
  /// @{
  explicit Tree(float64_t val);
  explicit Tree(std::floating_point auto val)
      : Tree(static_cast<float64_t>(val)) {}
  /// @}

  /// Construct a string node.
  /// @{
  explicit Tree(std::string_view val);
  explicit Tree(const std::convertible_to<std::string_view> auto& val)
      : Tree(std::string_view{val}) {} // NOLINT(*-array-to-pointer-decay)
  /// @}

  /// Construct an array node.
  explicit Tree(Array val);

  /// Construct a map node.
  explicit Tree(Map val);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Is this a null node?
  auto is_null() const noexcept -> bool;

  /// Is this a boolean node?
  auto is_bool() const noexcept -> bool;

  /// Is this an integer node?
  auto is_int() const noexcept -> bool;

  /// Is this a real number node?
  auto is_real() const noexcept -> bool;

  /// Is this a string node?
  auto is_string() const noexcept -> bool;

  /// Is this an array node?
  auto is_array() const noexcept -> bool;

  /// Is this a map node?
  auto is_map() const noexcept -> bool;

  /// Return a human-readable name of the current node kind.
  auto type_name() const noexcept -> std::string_view;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Return the value as a boolean.
  auto as_bool() const -> bool;

  /// Return the value as a 64-bit integer.
  auto as_int() const -> std::int64_t;

  /// Return the value as a 64-bit floating-point number.
  auto as_real() const -> float64_t;

  /// Return the value as a string view.
  auto as_string() const -> std::string_view;

  /// Return the value as an array.
  /// @{
  auto as_array() -> Array&;
  auto as_array() const -> const Array&;
  /// @}

  /// Return the value as a map.
  /// @{
  auto as_map() -> Map&;
  auto as_map() const -> const Map&;
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Number of elements (array) or fields (map) in this node.
  auto size() const -> std::size_t;

  /// Return the element at @p index.
  /// @{
  auto get(std::size_t index) -> Tree&;
  auto get(std::size_t index) const -> const Tree&;
  /// @}

  /// Return true if this map node contains the given key.
  auto has(std::string_view key) const -> bool;

  /// Return the child with @p key.
  /// @{
  auto get(std::string_view key) -> Tree&;
  auto get(std::string_view key) const -> const Tree&;
  /// @}

  /// Erase the child with @p key.
  void erase(std::string_view key);

  /// Return the keys of this map node.
  auto keys() const -> std::vector<std::string>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Deep-merge @p other into this tree.
  /// If this and @p other are both maps, their keys are merged recursively.
  /// Otherwise, the value is replaced.
  void merge(Tree other);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Assign a boolean value.
  void assign(bool val);

  /// Assign an integer value.
  /// @{
  void assign(std::int64_t val);
  void assign(std::integral auto val) {
    assign(static_cast<std::int64_t>(val));
  }

  /// Assign a real value.
  /// @{
  void assign(float64_t val);
  void assign(std::floating_point auto val) {
    assign(static_cast<float64_t>(val));
  }
  /// @}

  /// Assign a string value.
  /// @{
  void assign(std::string_view val);
  void assign(const std::convertible_to<std::string_view> auto& val) {
    assign(std::string_view{val}); // NOLINT(*-array-to-pointer-decay)
  }
  /// @}

  /// Assign an array.
  void assign(Array val);

  /// Assign a map.
  void assign(Map val);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Check if two trees are equal.
  auto operator==(const Tree& other) const -> bool = default;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  std::variant< //
      std::monostate,
      bool,
      std::int64_t,
      float64_t,
      std::string,
      Array,
      Map>
      data_;

}; // class Tree

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Serialize a property tree to a pretty JSON string.
auto tree_dump_json(const Tree& tree) -> std::string;

/// Deserialize a property tree from a JSON string.
auto tree_from_json(std::string_view string) -> Tree;

/// Serialize a property tree to a pretty YAML string.
auto tree_dump_yaml(const Tree& tree) -> std::string;

/// Deserialize a property tree from a YAML string.
auto tree_from_yaml(std::string_view string) -> Tree;

/// Deserialize a property tree from a file.
auto tree_from_file(const std::filesystem::path& path) -> Tree;

/// Serialize a property tree to a file.
void tree_dump_file(const Tree& tree, const std::filesystem::path& path);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
