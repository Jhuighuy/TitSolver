/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
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
