/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <Python.h> // IWYU pragma: keep

#include "tit/py/cast.hpp"
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
