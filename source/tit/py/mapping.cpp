/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <functional>
#include <string_view>

#include "tit/core/missing.hpp" // IWYU pragma: keep
#include "tit/core/str_utils.hpp"

#include "tit/py/_python.hpp"
#include "tit/py/error.hpp"
#include "tit/py/mapping.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Mapping::isinstance(const Object& obj) -> bool {
  return ensure(PyMapping_Check(obj.get()));
}

auto Mapping::has_key(const Object& key) const -> bool {
  return ensure(PyMapping_HasKey(get(), key.get()));
}
auto Mapping::has_key(CStrView key) const -> bool {
  return ensure(PyMapping_HasKeyString(get(), key.c_str()));
}

auto Mapping::at(CStrView key) const -> Object {
  return steal(ensure(PyMapping_GetItemString(get(), key.c_str())));
}
void Mapping::set_at(CStrView key, const Object& value) const {
  ensure(PyMapping_SetItemString(get(), key.c_str(), value.get()));
}

void Mapping::del(CStrView key) const {
  ensure(PyMapping_DelItemString(get(), key.c_str()));
}

auto Mapping::keys() const -> List {
  return steal<List>(ensure(PyMapping_Keys(get())));
}

auto Mapping::values() const -> List {
  return steal<List>(ensure(PyMapping_Values(get())));
}

auto Mapping::items() const -> List {
  return steal<List>(ensure(PyMapping_Items(get())));
}

Mapping::Mapping(PyObject* ptr) : Object{ptr} {
  TIT_ASSERT(isinstance(*this), "Object is not a mapping!");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Dict::type() -> Type {
  return borrow(&PyDict_Type);
}

auto Dict::isinstance(const Object& obj) -> bool {
  return ensure(PyDict_Check(obj.get()));
}

Dict::Dict() : Mapping{PyDict_New()} {}

Dict::Dict(const Object& mapping_or_iterable) : Dict{} {
  update(mapping_or_iterable);
}

void Dict::clear() const {
  PyDict_Clear(get()); // never fails.
}

void Dict::update(const Object& mapping_or_iterable) const {
  auto self = *this;
  self |= mapping_or_iterable;
  TIT_ASSERT(self.is(*this), "Dictionary is immutable?");
}

void Dict::for_each(
    std::move_only_function<void(const Object&, const Object&)> func) const {
  Py_ssize_t pos = 0;
  PyObject* key = nullptr;
  PyObject* value = nullptr;
  while (ensure(PyDict_Next(get(), &pos, &key, &value))) {
    std::invoke(func, borrow(key), borrow(value));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Set::type() -> Type {
  return borrow(&PySet_Type);
}

auto Set::isinstance(const Object& obj) -> bool {
  return ensure(PySet_Check(obj.get()));
}

Set::Set() : Object{ensure(PySet_New(nullptr))} {}

Set::Set(const Object& iterable) : Object{ensure(PySet_New(iterable.get()))} {}

void Set::clear() const {
  PySet_Clear(get()); // never fails.
}

auto Set::has(const Object& value) const -> bool {
  return ensure(PySet_Contains(get(), value.get()));
}

void Set::add(const Object& value) const {
  ensure(PySet_Add(get(), value.get()));
}

void Set::discard(const Object& value) const {
  ensure(PySet_Discard(get(), value.get()));
}

auto Set::pop() const -> Object {
  return steal(ensure(PySet_Pop(get())));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
