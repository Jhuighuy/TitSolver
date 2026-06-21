/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <variant>

#include "tit/core/assert.hpp"
#include "tit/core/str.hpp"
#include "tit/prop/path.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Path::child(std::string_view field) const -> Path {
  TIT_ASSERT(str_is_identifier(field),
             "Path field segment must be a valid identifier.");
  auto path{*this};
  path.segments_.emplace_back(std::string{field});
  return path;
}

auto Path::child(std::size_t index) const -> Path {
  auto path{*this};
  path.segments_.emplace_back(index);
  return path;
}

auto Path::parent() const -> Path {
  auto path{*this};
  if (!path.segments_.empty()) path.segments_.pop_back();
  return path;
}

auto Path::empty() const -> bool {
  return segments_.empty();
}

auto Path::size() const -> std::size_t {
  return segments_.size();
}

auto Path::operator[](std::size_t index) const -> const Segment& {
  TIT_ASSERT(index < segments_.size(), "Path index out of range.");
  return segments_[index];
}

auto Path::string() const -> std::string {
  if (segments_.empty()) return "/";
  return segments_ | std::views::transform([](const Segment& segment) {
           return std::visit(
               [](const auto& val) { return std::format("/{}", val); },
               segment);
         }) |
         std::views::join | std::ranges::to<std::string>();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
