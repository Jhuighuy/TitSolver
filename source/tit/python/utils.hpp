/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <utility>

#include "tit/core/checks.hpp"

namespace tit::py {

template<class To = class Object, class From>
auto cast(const From& arg) -> To;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference wrapper to the sequence item or slice.
template<class Object, class Index>
class ItemAt {
public:

  /// Construct a wrapper.
  constexpr ItemAt(const Object& self, Index index)
      : self_{&self}, index_{std::move(index)} {}

  /// Get the item or slice.
  template<class Value>
  constexpr explicit(false) operator Value() const {
    TIT_ASSERT(self_ != nullptr, "Sequence is null!");
    return cast<Value>(self_->at(index_));
  }

  /// Assign the item or slice.
  auto operator=(const auto& value) -> ItemAt& {
    TIT_ASSERT(self_ != nullptr, "Sequence is null!");
    self_->set_at(index_, value);
    return *this;
  }

private:

  const Object* self_;
  Index index_;

}; // class ItemAt

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
