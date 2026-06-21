/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Structured path to a property tree node.
class Path final {
public:

  /// Path segment.
  using Segment = std::variant<std::string, std::size_t>;

  /// Construct an empty root path.
  Path() = default;

  /// Construct a child field path.
  auto child(std::string_view field) const -> Path;

  /// Construct a child array element path.
  auto child(std::size_t index) const -> Path;

  /// Construct the parent path.
  auto parent() const -> Path;

  /// Check if path is empty.
  auto empty() const -> bool;

  /// Number of path segments.
  auto size() const -> std::size_t;

  /// Path segment at @p index.
  auto operator[](std::size_t index) const -> const Segment&;

  /// Format as a user-facing path string.
  auto string() const -> std::string;

  /// Compare paths.
  auto operator<=>(const Path&) const = default;

  /// Compare paths while treating all array indices as wildcards.
  auto same_pattern_as(const Path& other) const -> bool;

private:

  std::vector<Segment> segments_;

}; // class Path

/// Construct a path from a sequence of segments.
template<std::convertible_to<Path::Segment>... Segments>
auto make_path(Segments... segments) -> Path {
  Path result;
  ((result = result.child(std::move(segments))), ...);
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop

template<>
struct std::formatter<tit::prop::Path> : std::formatter<std::string> {
  auto format(const tit::prop::Path& path, std::format_context& context) const {
    return std::formatter<std::string>::format(path.string(), context);
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
