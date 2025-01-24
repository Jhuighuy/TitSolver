/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <optional>

#include "tit/core/str_utils.hpp"

#include "tit/py/object.hpp"

namespace tit::py {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python iterator reference.
class Iterator final : public Object {
public:

  /// Get the type name of the `Iterator` protocol.
  static constexpr CStrView type_name = "iterator";

  /// Check if the object implements the iterator protocol.
  static auto isinstance(const Object& obj) -> bool;

  /// Get the next item, similar to `next(iterator)`.
  auto next() const -> std::optional<Object>;

}; // class Iterator

/// Iterate over the iterable object.
auto iter(const Object& iterable) -> Iterator;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
