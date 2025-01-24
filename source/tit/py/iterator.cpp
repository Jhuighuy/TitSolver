/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <optional>

#include "tit/py/_python.hpp"
#include "tit/py/error.hpp"
#include "tit/py/iterator.hpp"
#include "tit/py/object.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Iterator::isinstance(const Object& obj) -> bool {
  return ensure(PyIter_Check(obj.get()));
}

auto Iterator::next() const -> std::optional<Object> {
  if (auto* const item = PyIter_Next(get()); item != nullptr) {
    return steal(item);
  }
  ensure_no_error();
  return std::nullopt;
}

auto iter(const Object& iterable) -> Iterator {
  return steal<Iterator>(ensure(PyObject_GetIter(iterable.get())));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
