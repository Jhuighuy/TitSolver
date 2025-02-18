/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <ranges>
#include <span>
#include <string_view>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/missing.hpp" // IWYU pragma: keep
#include "tit/core/str_utils.hpp"
#include "tit/core/tuple_utils.hpp"
#include "tit/core/uint_utils.hpp"

#include "tit/py/_python.hpp"
#include "tit/py/error.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Sequence::isinstance(const Object& obj) -> bool {
  return ensure(PySequence_Check(obj.get()));
}

auto Sequence::at(size_t index) const -> Object {
  return steal(ensure(PySequence_GetItem(get(), to_signed(index))));
}
void Sequence::set_at(size_t index, const Object& value) const {
  ensure(PySequence_SetItem(get(), to_signed(index), value.get()));
}

auto Sequence::at(pair_of_t<size_t> slice) const -> Sequence {
  return steal<Sequence>(ensure(PySequence_GetSlice(get(),
                                                    to_signed(slice.first),
                                                    to_signed(slice.second))));
}
void Sequence::set_at(pair_of_t<size_t> slice, const Object& values) const {
  ensure(PySequence_SetSlice(get(),
                             to_signed(slice.first),
                             to_signed(slice.second),
                             values.get()));
}

void Sequence::del(size_t index) const {
  ensure(PySequence_DelItem(get(), to_signed(index)));
}

void Sequence::del(pair_of_t<size_t> slice) const {
  ensure(PySequence_DelSlice(get(),
                             to_signed(slice.first),
                             to_signed(slice.second)));
}

auto Sequence::count(const Object& value) const -> size_t {
  return ensure<size_t>(PySequence_Count(get(), value.get()));
}

auto Sequence::contains(const Object& value) const -> bool {
  return ensure(PySequence_Contains(get(), value.get()));
}

auto Sequence::index(const Object& value) const -> size_t {
  return ensure<size_t>(PySequence_Index(get(), value.get()));
}

auto operator*(size_t n, const Sequence& a) -> Sequence {
  return a * n;
}
auto operator*(const Sequence& a, size_t n) -> Sequence {
  return steal<Sequence>(ensure(PySequence_Repeat(a.get(), to_signed(n))));
}
auto operator*=(Sequence& a, size_t n) -> Sequence& {
  a = steal<Sequence>(ensure(PySequence_InPlaceRepeat(a.get(), to_signed(n))));
  return a;
}

Sequence::Sequence(PyObject* ptr) : Object{ptr} {
  TIT_ASSERT(isinstance(*this), "Object is not a sequence!");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Str::type() -> Type {
  return borrow(&PyUnicode_Type);
}

auto Str::isinstance(const Object& obj) -> bool {
  return ensure(PyUnicode_Check(obj.get()));
}

Str::Str(std::string_view str)
    : Sequence{ensure( // NOLINTNEXTLINE(*-suspicious-stringview-data-usage)
          PyUnicode_FromStringAndSize(str.data(), to_signed(str.size())))} {}

Str::Str(const Object& obj) : Sequence{ensure(PyObject_Str(obj.get()))} {}

auto Str::val() const -> CStrView {
  Py_ssize_t size = 0;
  const auto* const result = PyUnicode_AsUTF8AndSize(get(), &size);
  ensure_no_error();
  TIT_ASSERT(result != nullptr, "String is null, but no error occurred!");
  return CStrView{result, to_unsigned(size)};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Tuple::type() -> Type {
  return borrow(&PyTuple_Type);
}

auto Tuple::isinstance(const Object& obj) -> bool {
  return ensure(PyTuple_Check(obj.get()));
}

Tuple::Tuple() : Sequence{ensure(PyTuple_New(0))} {}

// Note: despite the name, `PySequence_Tuple` actually accepts iterables.
Tuple::Tuple(const Object& iterable)
    : Sequence{ensure(PySequence_Tuple(iterable.get()))} {}

auto to_tuple(std::span<const Object> values) -> Tuple {
  // We cannot assign the items with `operator[]` because it would call
  // `PySequence_SetItem`, which will trigger a `TypeError`:
  // "'tuple' object does not support item assignment".
  auto result = steal<Tuple>(ensure(PyTuple_New(to_signed(values.size()))));
  for (const auto& [index, value] : std::views::enumerate(values)) {
    ensure(PyTuple_SetItem(result.get(),
                           static_cast<Py_ssize_t>(index),
                           Py_NewRef(value.get())));
  }
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto List::type() -> Type {
  return borrow(&PyList_Type);
}

auto List::isinstance(const Object& obj) -> bool {
  return ensure(PyList_Check(obj.get()));
}

List::List() : Sequence{ensure(PyList_New(0))} {}

// Note: despite the name, `PySequence_List` actually accepts iterables.
List::List(const Object& iterable)
    : Sequence{ensure(PySequence_List(iterable.get()))} {}

void List::insert(size_t index, const Object& value) const {
  ensure(PyList_Insert(get(), to_signed(index), value.get()));
}

void List::append(const Object& value) const {
  ensure(PyList_Append(get(), value.get()));
}

void List::sort() const {
  ensure(PyList_Sort(get()));
}

void List::reverse() const {
  ensure(PyList_Reverse(get()));
}

auto to_list(std::span<const Object> values) -> List {
  // We cannot assign the items with `operator[]` because it would call
  // `PySequence_SetItem`, which cannot be used to assign to a partially
  // initialized list.
  auto result = steal<List>(ensure(PyList_New(to_signed(values.size()))));
  for (const auto& [index, value] : std::views::enumerate(values)) {
    ensure(PyList_SetItem(result.get(),
                          static_cast<Py_ssize_t>(index),
                          Py_NewRef(value.get())));
  }
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
