/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/py/_python.hpp"
#include "tit/py/capsule.hpp"
#include "tit/py/error.hpp"
#include "tit/py/object.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Capsule::type() -> Type {
  return borrow(&PyCapsule_Type);
}

auto Capsule::isinstance(const Object& obj) -> bool {
  return ensure(PyCapsule_CheckExact(obj.get()));
}

Capsule::Capsule(void* data, CapsuleDestructor destructor)
    : Object{ensure(PyCapsule_New(data, /*name=*/nullptr, destructor))} {}

auto Capsule::data() const -> void* {
  return ensure(PyCapsule_GetPointer(get(), /*name=*/nullptr));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
