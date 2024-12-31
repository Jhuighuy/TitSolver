/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/python/python_h.hpp" // must be first.

#include <concepts>
#include <optional>
#include <utility>

#include "tit/core/checks.hpp"
#include "tit/core/str_utils.hpp"
#include "tit/core/uint_utils.hpp"
#include "tit/core/utils.hpp"

#include "tit/python/casts.hpp"
#include "tit/python/errors.hpp"
#include "tit/python/utils.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python object smart pointer. Unlike `Object`, may be null.
class ObjectPtr {
public:

  /// Construct a pointer to the existing object.
  explicit ObjectPtr(PyObject* obj) noexcept : obj_{obj} {}

  /// Move-construct a pointer.
  ObjectPtr(ObjectPtr&& other) noexcept : obj_{other.release()} {}

  /// Copy-construct a pointer.
  ObjectPtr(const ObjectPtr& other) noexcept : obj_{Py_XNewRef(other.get())} {}

  /// Move-assign a pointer.
  auto operator=(ObjectPtr&& other) noexcept -> ObjectPtr& {
    if (this != &other) reset(other.release());
    return *this;
  }

  /// Copy-assign a pointer.
  auto operator=(const ObjectPtr& other) noexcept -> ObjectPtr& {
    if (this != &other) reset(Py_XNewRef(other.get()));
    return *this;
  }

  /// Destroy the pointer.
  ~ObjectPtr() noexcept {
    Py_CLEAR(obj_);
  }

  /// Reset the pointer.
  void reset(PyObject* obj) noexcept {
    Py_CLEAR(obj_);
    obj_ = obj;
  }

  /// Release the pointer.
  auto release() noexcept -> PyObject* {
    return std::exchange(obj_, nullptr);
  }

  /// Check if the pointer is valid.
  auto valid() const noexcept -> bool {
    return obj_ != nullptr;
  }

  /// Get pointer to the object, can be null.
  auto get() const noexcept -> PyObject* {
    return obj_;
  }

  /// Get pointer to the object, can not be null.
  /// @{
  explicit(false) operator PyObject*() const noexcept {
    TIT_ASSERT(valid(), "Object is null!");
    return get();
  }
  auto operator->() const noexcept -> PyObject* {
    TIT_ASSERT(valid(), "Object is null!");
    return get();
  }
  /// @}

private:

  PyObject* obj_;

}; // class ObjectPtr

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python object reference.
class Object : public ObjectPtr {
public:

  /// Construct a reference to the existing object.
  /// @{
  explicit Object(PyObject* obj) : ObjectPtr(ensure(obj)) {}
  explicit Object(const Object& obj, keep_t /*tag*/) : ObjectPtr{obj} {}
  /// @}

  /// Check if the object has an attribute with the given name.
  /// @{
  auto has_attr(const Object& name) const -> bool {
    return ensure<bool>(PyObject_HasAttr(get(), name));
  }
  auto has_attr(CStrView name) const -> bool {
    return ensure<bool>(PyObject_HasAttrString(get(), name.c_str()));
  }
  /// @}

  /// Access the object attribute, similar to `obj.attr`.
  /// @{
  auto attr(const Object& name) const -> Object {
    return Object{PyObject_GetAttr(get(), name)};
  }
  auto attr(CStrView name) const -> Object {
    return Object{PyObject_GetAttrString(get(), name.c_str())};
  }
  void set_attr(const Object& name, const auto& value) const {
    ensure(PyObject_SetAttr(get(), name, cast(value)));
  }
  void set_attr(CStrView name, const auto& value) const {
    ensure(PyObject_SetAttrString(get(), name.c_str(), cast(value)));
  }
  /// @}

  /// Delete the object attribute, similar to `del obj.attr`.
  /// @{
  void del_attr(const Object& name) const {
    ensure(PyObject_DelAttr(get(), name));
  }
  void del_attr(CStrView name) const {
    ensure(PyObject_DelAttrString(get(), name.c_str()));
  }
  /// @}

  /// Access the item with the given key, similar to `obj[key]`.
  /// @{
  auto at(const Object& key) const -> Object {
    return Object{PyObject_GetItem(get(), key)};
  }
  void set_at(const Object& key, const auto& value) const {
    ensure(PyObject_SetItem(get(), key, cast(value)));
  }
  auto operator[](const Object& key) const -> ItemAt<Object, Object> {
    return {*this, key};
  }
  /// @}

  /// Delete the item with the given key, similar to `del obj[key]`.
  void del(const Object& key) const {
    ensure(PyObject_DelItem(get(), key));
  }

  /// Invoke the object via the `tp_call` protocol.
  /// @{
  auto tp_call() const -> Object {
    return Object{PyObject_CallNoArgs(get())};
  }
  auto tp_call(const Object& args) const -> Object {
    return Object{PyObject_CallObject(get(), args)};
  }
  auto tp_call(const Object& args, const Object& kwargs) const -> Object {
    return Object{PyObject_Call(get(), args, kwargs)};
  }
  auto operator()() const -> Object {
    return tp_call();
  }
  auto operator()(const auto&... args) const -> Object; // defined below.
  /// @}

  /// Check if the object represents a true value, similar to `bool(obj)`.
  explicit operator bool() const {
    return ensure<bool>(PyObject_IsTrue(get()));
  }

  /// Check if the object represents a false value, similar to `not obj`.
  auto operator!() const -> bool {
    return ensure<bool>(PyObject_Not(get()));
  }

  /// Check if the object is another object.
  /// @{
  auto is(const Object& other) const -> bool {
    return Py_Is(get(), other.get());
  }
  auto is_not(const Object& other) const -> bool {
    return !is(other);
  }
  /// @}

  /// Comparison operators.
  /// @{
  friend auto operator==(const Object& a, const Object& b) -> bool {
    return ensure<bool>(PyObject_RichCompareBool(a, b, Py_EQ));
  }
  friend auto operator!=(const Object& a, const Object& b) -> bool {
    return ensure<bool>(PyObject_RichCompareBool(a, b, Py_NE));
  }
  friend auto operator<(const Object& a, const Object& b) -> bool {
    return ensure<bool>(PyObject_RichCompareBool(a, b, Py_LT));
  }
  friend auto operator<=(const Object& a, const Object& b) -> bool {
    return ensure<bool>(PyObject_RichCompareBool(a, b, Py_LE));
  }
  friend auto operator>(const Object& a, const Object& b) -> bool {
    return ensure<bool>(PyObject_RichCompareBool(a, b, Py_GT));
  }
  friend auto operator>=(const Object& a, const Object& b) -> bool {
    return ensure<bool>(PyObject_RichCompareBool(a, b, Py_GE));
  }
  /// @}

  /// Arithmetic operations.
  /// @{
  friend auto operator+(const Object& a) -> Object {
    return Object{PyNumber_Positive(a)};
  }
  friend auto operator-(const Object& a) -> Object {
    return Object{PyNumber_Negative(a)};
  }
  friend auto operator+(const Object& a, const Object& b) -> Object {
    return Object{PyNumber_Add(a, b)};
  }
  friend auto operator-(const Object& a, const Object& b) -> Object {
    return Object{PyNumber_Subtract(a, b)};
  }
  friend auto operator*(const Object& a, const Object& b) -> Object {
    return Object{PyNumber_Multiply(a, b)};
  }
  friend auto operator/(const Object& a, const Object& b) -> Object {
    return Object{PyNumber_TrueDivide(a, b)};
  }
  friend auto operator%(const Object& a, const Object& b) -> Object {
    return Object{PyNumber_Remainder(a, b)};
  }
  friend auto operator+=(Object& a, const Object& b) -> Object& {
    ensure_decref(PyNumber_InPlaceAdd(a, b));
    return a;
  }
  friend auto operator-=(Object& a, const Object& b) -> Object& {
    ensure_decref(PyNumber_InPlaceSubtract(a, b));
    return a;
  }
  friend auto operator*=(Object& a, const Object& b) -> Object& {
    ensure_decref(PyNumber_InPlaceMultiply(a, b));
    return a;
  }
  friend auto operator/=(Object& a, const Object& b) -> Object& {
    ensure_decref(PyNumber_InPlaceTrueDivide(a, b));
    return a;
  }
  friend auto operator%=(Object& a, const Object& b) -> Object& {
    ensure_decref(PyNumber_InPlaceRemainder(a, b));
    return a;
  }
  /// @}

  /// Bitwise operations.
  /// @{
  friend auto operator~(const Object& a) -> Object {
    return Object{PyNumber_Invert(a)};
  }
  friend auto operator&(const Object& a, const Object& b) -> Object {
    return Object{PyNumber_And(a, b)};
  }
  friend auto operator|(const Object& a, const Object& b) -> Object {
    return Object{PyNumber_Or(a, b)};
  }
  friend auto operator^(const Object& a, const Object& b) -> Object {
    return Object{PyNumber_Xor(a, b)};
  }
  friend auto operator<<(const Object& a, const Object& b) -> Object {
    return Object{PyNumber_Lshift(a, b)};
  }
  friend auto operator>>(const Object& a, const Object& b) -> Object {
    return Object{PyNumber_Rshift(a, b)};
  }
  friend auto operator&=(Object& a, const Object& b) -> Object& {
    ensure_decref(PyNumber_InPlaceAnd(a, b));
    return a;
  }
  friend auto operator|=(Object& a, const Object& b) -> Object& {
    ensure_decref(PyNumber_InPlaceOr(a, b));
    return a;
  }
  friend auto operator^=(Object& a, const Object& b) -> Object& {
    ensure_decref(PyNumber_InPlaceXor(a, b));
    return a;
  }
  friend auto operator<<=(Object& a, const Object& b) -> Object& {
    ensure_decref(PyNumber_InPlaceLshift(a, b));
    return a;
  }
  friend auto operator>>=(Object& a, const Object& b) -> Object& {
    ensure_decref(PyNumber_InPlaceRshift(a, b));
    return a;
  }
  /// @}

}; // class Object

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python boolean object reference.
class Bool final : public Object {
public:

  /// Construct a new boolean object from C++ boolean.
  explicit Bool(bool value) : Object{PyBool_FromLong(value ? 1 : 0)} {}

  /// Construct a new boolean object from Python object, similar to `bool(obj)`.
  explicit Bool(const Object& obj) : Bool{static_cast<bool>(obj)} {}

  /// Construct a new boolean object from Python object, or
  /// keep the existing object if it already is a subclass of `bool`.
  explicit Bool(const Object& obj, keep_t /*keep*/)
      : Object{ensure<bool>(PyBool_Check(obj)) ? obj : Bool{obj}} {}

  /// Get the C++ boolean value.
  auto val() const -> bool {
    return ensure<bool>(PyObject_IsTrue(get()));
  }

}; // class Bool

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python integer object reference.
class Int final : public Object {
public:

  /// Construct a new integer object from C++ integer.
  explicit Int(std::integral auto value)
      : Object{PyLong_FromLongLong(static_cast<long long>(value))} {}

  /// Construct a new integer object from Python object, similar to `int(obj)`.
  explicit Int(const Object& obj) : Object{PyNumber_Long(obj)} {}

  /// Construct a new integer object from Python object, or
  /// keep the existing object if it already is a subclass of `int`.
  explicit Int(const Object& obj, keep_t /*keep*/)
      : Object{ensure<bool>(PyLong_Check(obj)) ? obj : Int{obj}} {}

  /// Get the C++ integer value.
  auto val() const -> long long {
    const auto result = PyLong_AsLongLong(get());
    if (result == -1 && PyErr_Occurred() != nullptr) raise();
    return result;
  }

}; // class Int

auto int_(std::integral auto value) -> Int {
  return Int{value};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python float object reference.
class Float final : public Object {
public:

  /// Construct a new float object from C++ floating-point value.
  explicit Float(std::floating_point auto value)
      : Object{PyFloat_FromDouble(static_cast<double>(value))} {}

  /// Construct a new float object from Python object, similar to `float(obj)`.
  explicit Float(const Object& obj) : Object{PyNumber_Float(obj)} {}

  /// Construct a new float object from Python object, or
  /// keep the existing object if it already is a subclass of `float`.
  explicit Float(const Object& obj, keep_t /*tag*/)
      : Object{ensure<bool>(PyFloat_Check(obj)) ? obj : Float{obj}} {}

  /// Get the C++ floating-point value.
  auto val() const -> double {
    const auto result = PyFloat_AsDouble(get());
    if (result == -1.0 && PyErr_Occurred() != nullptr) raise();
    return result;
  }

}; // class Float

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python iterator reference.
class Iterator final : public Object {
public:

  /// Construct a new reference to the existing iterator object.
  explicit Iterator(const Object& iterator, keep_t /*tag*/ = {})
      : Object{iterator} {
    if (PyIter_Check(get()) == 0) raise_type_error("not an iterator");
  }

  /// Get the next item, similar to `next(iterator)`.
  auto next() const -> std::optional<Object> {
    if (auto* const item = PyIter_Next(get()); item != nullptr) {
      return Object{item};
    }
    if (PyErr_Occurred() == nullptr) return std::nullopt;
    raise();
  }

}; // class Iterator

/// Iterate over the iterable object.
inline auto iter(const Object& iterable) -> Iterator {
  return cast<Iterator>(Object{PyObject_GetIter(iterable)});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python sequence reference.
class Sequence : public Object {
public:

  using Object::at;
  using Object::del;
  using Object::set_at;
  using Object::operator[];

  /// Construct a new reference to the existing sequence object.
  explicit Sequence(const Object& sequence, keep_t /*tag*/ = {})
      : Object{sequence} {
    if (PySequence_Check(get()) == 0) raise_type_error("not a sequence");
  }

  /// Access an item in the sequence.
  /// @{
  auto at(size_t index) const -> Object {
    return Object{PySequence_GetItem(get(), to_signed(index))};
  }
  void set_at(size_t index, const auto& item) const {
    ensure(PySequence_SetItem(get(), to_signed(index), cast(item)));
  }
  auto operator[](size_t index) const -> ItemAt<Sequence, size_t> {
    return {*this, index};
  }
  /// @}

  /// Access a slice of the sequence.
  /// @{
  auto at(pair_of_t<size_t> slice) const -> Sequence {
    return cast<Sequence>(Object{PySequence_GetSlice(get(),
                                                     to_signed(slice.first),
                                                     to_signed(slice.second))});
  }
  void set_at(pair_of_t<size_t> slice, const auto& values) const {
    ensure(PySequence_SetSlice(get(),
                               to_signed(slice.first),
                               to_signed(slice.second),
                               cast(values)));
  }
  auto operator[](pair_of_t<size_t> slice) const
      -> ItemAt<Sequence, pair_of_t<size_t>> {
    return {*this, slice};
  }
  /// @}

  /// Delete the item.
  void del(size_t index) const {
    ensure(PySequence_DelItem(get(), to_signed(index)));
  }

  /// Delete the slice.
  void del(pair_of_t<size_t> slice) const {
    ensure(PySequence_DelSlice(get(),
                               to_signed(slice.first),
                               to_signed(slice.second)));
  }

  /// Count the number of occurrences of the item in the sequence.
  auto count(const auto& item) const -> size_t {
    return ensure<size_t>(PySequence_Count(get(), cast(item)));
  }

  /// Check that the sequence contains the given item.
  auto contains(const auto& item) const -> bool {
    return PySequence_Contains(get(), cast(item)) != 0;
  }

  /// Find the index of the item in the sequence.
  auto index(const auto& item) const -> size_t {
    return ensure<size_t>(PySequence_Index(get(), cast(item)));
  }

}; // class Sequence

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python string reference.
class Str final : public Sequence {
public:

  /// Construct a new empty string object.
  Str() : Str("") {}

  /// Construct a new string object from C++ string.
  explicit Str(std::string_view str)
      : Sequence{Object{
            // NOLINTNEXTLINE(*-suspicious-stringview-data-usage)
            PyUnicode_FromStringAndSize(str.data(), to_signed(str.size()))}} {}

  /// Construct a new float object from Python object, similar to `str(obj)`.
  explicit Str(const Object& obj, keep_t /*tag*/)
      : Sequence{Object{PyObject_Str(obj)}} {}

  /// Get the C++ string.
  auto val() const -> std::string_view {
    Py_ssize_t size = 0;
    const auto* const ptr = PyUnicode_AsUTF8AndSize(get(), &size);
    if (ptr == nullptr) raise();
    return {ptr, to_unsigned(size)};
  }

}; // class Str

/// Construct a new empty string.
inline auto str() -> Str {
  return Str{};
}

/// Construct a string object from C++ string.
inline auto str(std::string_view str) -> Str {
  return Str{str};
}

/// String representation, similar to `str(obj)`.
inline auto str(const Object& obj, keep_t tag = {}) -> Str {
  return Str{obj, tag};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python tuple.
class Tuple final : public Sequence {
public:

  /// Construct an empty tuple object.
  Tuple() : Sequence{Object{PyTuple_New(0)}} {}

  /// Construct a tuple object from Python object, similar to `tuple(obj)`.
  explicit Tuple(const Object& iterable, keep_t /*tag*/)
      // Note: despite the name, `PySequence_Tuple` actually accepts iterables.
      : Sequence{Object{PySequence_Tuple(iterable)}} {}

}; // class Tuple

/// Construct a new empty tuple.
inline auto tuple() -> Tuple {
  return Tuple{};
}

/// Construct a tuple from Python object, similar to `tuple(obj)`.
inline auto tuple(const Object& iterable, keep_t tag = {}) -> Tuple {
  return Tuple{iterable, tag};
}

/// Construct a new tuple object containing the given items.
auto make_tuple(const auto&... items) -> Tuple {
  // We cannot assign the items with `operator[]` because it would call
  // `PySequence_SetItem`, which will trigger a `TypeError`:
  // "'tuple' object does not support item assignment".
  //
  // Note: `PyTuple_SetItem` steals the reference to the item.
  Tuple tuple{Object{PyTuple_New(sizeof...(items))}, keep};
  Py_ssize_t index = 0;
  ((ensure<bool>(PyTuple_SetItem(tuple, index++, cast(items).release()))), ...);
  return tuple;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python list reference.
class List final : public Sequence {
public:

  /// Construct an empty list.
  List() : Sequence{Object{PyList_New(0)}} {}

  /// Construct a new list object from Python object, similar to `list(obj)`.
  /// If the object is already a list, it is copied.
  explicit List(const Object& iterable, copy_t /*tag*/)
      // Despite the name, `PySequence_List` accepts iterables.
      : Sequence{Object{PySequence_List(iterable)}} {}

  /// Construct a new list object from Python object, or
  /// keep the existing object if it already is a subclass of `list`.
  explicit List(const Object& iterable, keep_t /*tag*/)
      : Sequence{ensure<bool>(PyList_Check(iterable)) ? iterable :
                                                        List{iterable, copy}} {}

  /// Insert an item to the list.
  void insert(size_t index, const auto& item) const {
    ensure(PyList_Insert(get(), to_signed(index), cast(item)));
  }

  /// Append an item to the list.
  void append(const auto& item) const {
    ensure(PyList_Append(get(), cast(item)));
  }

  /// Sort the list.
  void sort() const {
    ensure(PyList_Sort(get()));
  }

  /// Reverse the list.
  void reverse() const {
    ensure(PyList_Reverse(get()));
  }

}; // class List

/// Construct a new empty list.
inline auto list() -> List {
  return List{};
}

/// Construct a list object from Python object, similar to `list(obj)`.
template<class CopyOrKeep = copy_t>
auto list(const Object& iterable, CopyOrKeep copy_or_keep = {}) -> List {
  return List{iterable, copy_or_keep};
}

/// Construct a new list object containing the given items.
auto make_list(const auto&... items) -> List {
  // We cannot assign the items with `operator[]` because it would call
  // `PySequence_SetItem`, which cannot be used to assign to a partially
  // initialized list.
  //
  // Note: `PyList_SetItem` steals the reference to the item.
  List list{Object{PyList_New(sizeof...(items))}, keep};
  Py_ssize_t index = 0;
  ((ensure<bool>(PyList_SetItem(list, index++, cast(items).release()))), ...);
  return list;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python object that implements the mapping protocol.
class Mapping : public Object {
public:

  using Object::at;
  using Object::del;
  using Object::set_at;
  using Object::operator[];

  /// Construct a new reference to the existing mapping object.
  explicit Mapping(const Object& mapping, keep_t /*tag*/ = {})
      : Object{mapping} {
    if (PyMapping_Check(get()) == 0) raise_type_error("not a mapping");
  }

  /// Check if the mapping contains the given key.
  /// @{
  auto has_key(const Object& key) const -> bool {
    return ensure<bool>(PyMapping_HasKey(get(), key));
  }
  auto has_key(CStrView key) const -> bool {
    return ensure<bool>(PyMapping_HasKeyString(get(), key.c_str()));
  }
  /// @}

  /// Access the item with the given key.
  /// @{
  auto at(CStrView key) const -> Object {
    return Object{PyMapping_GetItemString(get(), key.c_str())};
  }
  void set_at(CStrView key, const auto& value) const {
    ensure(PyMapping_SetItemString(get(), key.c_str(), cast(value)));
  }
  auto operator[](CStrView key) const -> ItemAt<Mapping, std::string> {
    return {*this, std::string{key}};
  }
  /// @}

  /// Delete the item with the given key.
  void del(CStrView key) const {
    ensure(PyMapping_DelItemString(get(), key.c_str()));
  }

  /// Keys of the mapping.
  auto keys() const -> List {
    return cast<List>(Object{PyMapping_Keys(get())});
  }

  /// Values of the mapping.
  auto values() const -> List {
    return cast<List>(Object{PyMapping_Values(get())});
  }

  /// Items of the mapping.
  auto items() const -> List {
    return cast<List>(Object{PyMapping_Items(get())});
  }

}; // class Mapping

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python dictionary.
class Dict final : public Mapping {
public:

  /// Construct a new empty dictionary.
  Dict() : Mapping{Object{PyDict_New()}} {}

  /// Construct a new dictionary object from Python object, similar to
  /// `dict(obj)`. If the object is already a dictionary, it is copied.
  explicit Dict(const Object& iterable, copy_t /*tag*/) : Dict{} {
    update(iterable);
  }

  /// Construct a new dictionary object from Python object,
  /// or keep the existing object if it already is a subclass of `dict`.
  explicit Dict(const Object& mapping_or_iterable, keep_t /*tag*/)
      : Mapping{ensure<bool>(PyDict_Check(mapping_or_iterable)) ?
                    mapping_or_iterable :
                    Dict{mapping_or_iterable, copy}} {}

  /// Clear the dict.
  void clear() const {
    PyDict_Clear(get()); // never fails.
  }

  /// Update the dict with the other mapping or iterable of key-value pairs.
  void update(const Object& mapping_or_iterable) const {
    // Note: Our implementation is equivalent to `*this |= mapping_or_iterable`.
    constexpr int override_ = 1;
    if (ensure<bool>(PyMapping_Check(mapping_or_iterable)) &&
        mapping_or_iterable.has_attr("keys")) {
      ensure(PyDict_Merge(get(), mapping_or_iterable, override_));
    } else {
      ensure(PyDict_MergeFromSeq2(get(), mapping_or_iterable, override_));
    }
  }

}; // class Dict

/// Construct a new empty dictionary.
inline auto dict() -> Dict {
  return Dict{};
}

/// Construct a dictionary object from Python object, similar to `dict(obj)`.
template<class CopyOrKeep = copy_t>
auto dict(const Object& mapping_or_iterable, CopyOrKeep copy_or_keep = {})
    -> Dict {
  return Dict{mapping_or_iterable, copy_or_keep};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python set.
class Set final : public Object {
public:

  /// Construct a new empty set object.
  Set() : Object{PySet_New(nullptr)} {}

  /// Construct a new set object from Python object, similar to `set(obj)`.
  /// If the object is already a set, it is copied.
  explicit Set(const Object& iterable, copy_t /*tag*/)
      : Object{PySet_New(iterable)} {}

  /// Construct a new set object from Python object,
  /// or keep the existing object if it already is a subclass of `set`.
  explicit Set(const Object& iterable, keep_t /*tag*/)
      // Note: `PySet_Check` actually returns bool, and not int, like others.
      : Object{PySet_Check(iterable) ? iterable : Set{iterable, copy}} {}

  /// Clear the set.
  void clear() const {
    PySet_Clear(get()); // never fails.
  }

  /// Check if the set contains the given item.
  auto has(const auto& item) const -> bool {
    return ensure<bool>(PySet_Contains(get(), cast(item)));
  }

  /// Add the item to the set.
  void add(const auto& item) const {
    ensure(PySet_Add(get(), cast(item)));
  }

  /// Remove the item from the set.
  void discard(const auto& item) const {
    ensure(PySet_Discard(get(), cast(item)));
  }

  /// Pop an item from the set.
  auto pop() const -> Object {
    return Object{PySet_Pop(get())};
  }

}; // class Set

/// Construct a new empty set object.
inline auto set() -> Set {
  return Set{};
}

/// Construct a set object from Python object, similar to `set(obj)`.
template<class CopyOrKeep = copy_t>
auto set(const Object& iterable, CopyOrKeep copy_or_keep = {}) -> Set {
  return Set{iterable, copy_or_keep};
}

/// Construct a new set object containing the given items.
auto make_set(const auto&... items) -> Set {
  Set set{};
  (set.add(items), ...);
  return set;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python module.
class Module final : public Object {
public:

  /// Construct a reference to the existing module.
  explicit Module(const Object& mod, keep_t /*tag*/ = {}) : Object{mod} {
    if (PyModule_Check(mod) == 0) raise_type_error("not a module");
  }

  /// Get the module dictionary.
  auto dict() const -> Dict {
    return cast<Dict>(Object{Py_XNewRef(PyModule_GetDict(get()))});
  }

}; // class Module

/// Import the module by name, similar to `import abc`.
inline auto import_(CStrView name) -> Module {
  return Module{Object{PyImport_ImportModule(name.c_str())}};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// `None` literal.
inline auto None() -> Object {
  return Object{Py_NewRef(Py_None)};
}

/// Length of the object, similar to `len(obj)`.
inline auto len(const Object& obj) -> size_t {
  return ensure<size_t>(PyObject_Length(obj));
}

/// Hash the object, similar to `hash(obj)`.
inline auto hash(const Object& obj) -> size_t {
  return ensure<size_t>(PyObject_Hash(obj));
}

/// Object representation, similar to `repr(obj)`.
inline auto repr(const Object& obj) -> Str {
  return cast<Str>(Object{PyObject_Repr(obj)});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Absolute value of the object, similar to `abs(obj)`.
inline auto abs(const Object& obj) -> Object {
  return Object{PyNumber_Absolute(obj)};
}

/// Matrix-multiplication of the objects, similar to `a @ b`.
/// @{
inline auto matmul(const Object& a, const Object& b) -> Object {
  return Object{PyNumber_MatrixMultiply(a, b)};
}
inline auto matmul_inplace(Object& a, const Object& b) -> Object& {
  ensure_decref(PyNumber_InPlaceMatrixMultiply(a, b));
  return a;
}
/// @}

/// Floor division of the objects, similar to `a // b`.
/// @{
inline auto floordiv(const Object& a, const Object& b) -> Object {
  return Object{PyNumber_FloorDivide(a, b)};
}
inline auto floordiv_inplace(Object& a, const Object& b) -> Object& {
  ensure_decref(PyNumber_InPlaceFloorDivide(a, b));
  return a;
}
/// @}

/// Power of the objects, similar to `a ** b`.
/// @{
inline auto pow(const Object& a, const Object& b, const Object& c = None())
    -> Object {
  return Object{PyNumber_Power(a, b, c)};
}
inline auto pow_inplace(Object& a, const Object& b, const Object& c = None())
    -> Object& {
  ensure_decref(PyNumber_InPlacePower(a, b, c));
  return a;
}
/// @}

/// Division quotient and remainder, similar to `divmod(a, b)`.
/// @{
inline auto divmod(const Object& a, const Object& b) -> Tuple {
  return cast<Tuple>(Object{PyNumber_Divmod(a, b)});
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
