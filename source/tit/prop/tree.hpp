/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "tit/core/basic_types.hpp"
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
  explicit Tree(int64_t val);
  explicit Tree(std::integral auto val) : Tree(static_cast<int64_t>(val)) {}
  /// @}

  /// Construct a real node.
  /// @{
  explicit Tree(float64_t val);
  explicit Tree(std::floating_point auto val)
      : Tree(static_cast<float64_t>(val)) {}
  /// @}

  /// Construct a string node.
  /// @{
  explicit Tree(std::string val);
  explicit Tree(std::convertible_to<std::string> auto val)
      : Tree(std::string{val}) {}
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
  auto as_int() const -> int64_t;

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
  auto size() const -> size_t;

  /// Return the element at @p index.
  /// @{
  auto get(size_t index) -> Tree&;
  auto get(size_t index) const -> const Tree&;
  /// @}

  /// Return true if this map node contains the given key.
  auto has(std::string_view key) const -> bool;

  /// Return the child with @p key.
  /// @{
  auto get(std::string_view key) -> Tree&;
  auto get(std::string_view key) const -> const Tree&;
  /// @}

  /// Return the keys of this map node.
  auto keys() const -> std::vector<std::string>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Deep-merge @p other into this tree.
  /// If this and @p other are both maps, their keys are merged recursively.
  /// Otherwise, the value is replaced.
  void merge(Tree other);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Set a boolean value.
  void set(bool val);

  /// Set an integer value.
  /// @{
  void set(int64_t val);
  void set(std::integral auto val) {
    set(static_cast<int64_t>(val));
  }

  /// Set a real value.
  /// @{
  void set(float64_t val);
  void set(std::floating_point auto val) {
    set(static_cast<float64_t>(val));
  }
  /// @}

  /// Set a string value.
  /// @{
  void set(std::string val);
  void set(std::convertible_to<std::string> auto val) {
    set(std::string{val});
  }
  /// @}

  /// Set an array.
  void set(Array val);

  /// Set a map.
  void set(Map val);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  std::variant< //
      std::monostate,
      bool,
      int64_t,
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
