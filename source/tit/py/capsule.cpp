/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/py/_python.hpp"
#include "tit/py/capsule.hpp"
#include "tit/py/error.hpp"
#include "tit/py/object.hpp"
#include "tit/py/type.hpp"

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
