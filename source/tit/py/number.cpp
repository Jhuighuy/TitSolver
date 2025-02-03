/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/py/_python.hpp"
#include "tit/py/error.hpp"
#include "tit/py/number.hpp"
#include "tit/py/object.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Bool::type() -> Type {
  return borrow(&PyBool_Type);
}

auto Bool::isinstance(const Object& obj) -> bool {
  return ensure(PyBool_Check(obj.get()));
}

Bool::Bool(bool value)
    : Object{ensure(PyBool_FromLong(static_cast<long>(value)))} {}

Bool::Bool(const Object& obj) : Bool{static_cast<bool>(obj)} {}

auto Bool::val() const -> bool {
  return ensure(PyObject_IsTrue(get()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Int::type() -> Type {
  return borrow(&PyLong_Type);
}

auto Int::isinstance(const Object& obj) -> bool {
  return ensure(PyLong_Check(obj.get()));
}

Int::Int(long long value) : Object{ensure(PyLong_FromLongLong(value))} {}

Int::Int(const Object& obj) : Object{ensure(PyNumber_Long(obj.get()))} {}

auto Int::val() const -> long long {
  const auto result = PyLong_AsLongLong(get());
  ensure_no_error();
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Float::type() -> Type {
  return borrow(&PyFloat_Type);
}

auto Float::isinstance(const Object& obj) -> bool {
  return ensure(PyFloat_Check(obj.get()));
}

Float::Float(double value) : Object{ensure(PyFloat_FromDouble(value))} {}

Float::Float(const Object& obj) : Object{ensure(PyNumber_Float(obj.get()))} {}

auto Float::val() const -> double {
  const auto result = PyFloat_AsDouble(get());
  ensure_no_error();
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
