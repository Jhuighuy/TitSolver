/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/py/_python.hpp"
#include "tit/py/error.hpp"
#include "tit/py/iterator.hpp"
#include "tit/py/object.hpp"
#include "tit/py/typing.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Iterator::isinstance(const Object& obj) -> bool {
  return ensure(PyIter_Check(obj.get()));
}

auto Iterator::next() const -> Optional<Object> {
  return maybe_steal(PyIter_Next(get()));
}

auto iter(const Object& iterable) -> Iterator {
  return steal<Iterator>(ensure(PyObject_GetIter(iterable.get())));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
