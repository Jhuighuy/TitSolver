/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/py/core.hpp"
#pragma once

#include <concepts>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "tit/core/str_utils.hpp"
#include "tit/core/uint_utils.hpp"
#include "tit/core/utils.hpp"

#include "tit/py/_core/_python.hpp"
#include "tit/py/_core/utils.hpp"

namespace tit::py {

class Object;
class Tuple;
class Dict;

namespace impl {
template<class>
struct Converter;
} // namespace impl

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Steal the reference to the object.
template<std::derived_from<Object> Derived = Object>
auto steal(PyObject* obj) -> Derived;

/// Borrow the reference to the object.
template<std::derived_from<Object> Derived = Object>
auto borrow(PyObject* obj) -> Derived;

/// Make a Python object from the given argument.
template<class Value>
auto object(Value&& value) -> Object;

/// Convert a Python object reference to another type.
template<std::derived_from<Object> Derived>
auto expect(const Object& arg) -> Derived; // defined in `errors.hpp`.

/// Extract the C++ value from the Python object.
template<class Value>
auto extract(const Object& obj) -> decltype(auto);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python object reference.
class Object : public ObjPtr {
public:

  using ObjPtr::ObjPtr;

  /// Check if the object is a subclass of `Object`.
  static constexpr auto isinstance(const Object& /*obj*/) -> bool {
    return true;
  }

  /// Check if the object is another object.
  auto is(const Object& other) const -> bool {
    return Py_Is(get(), other.get());
  }

  /// Check if the object has an attribute with the given name.
  /// @{
  auto has_attr(const Object& name) const -> bool {
    return ensure(PyObject_HasAttr(get(), name.get()));
  }
  auto has_attr(CStrView name) const -> bool {
    return ensure(PyObject_HasAttrString(get(), name.c_str()));
  }
  /// @}

  /// Access the object attribute, similar to `obj.attr`.
  /// @{
  auto attr(const Object& name) const -> Object {
    return steal(PyObject_GetAttr(get(), name.get()));
  }
  auto attr(CStrView name) const -> Object {
    return steal(PyObject_GetAttrString(get(), name.c_str()));
  }
  template<class Value>
  void set_attr(const Object& name, Value&& value) const {
    ensure(PyObject_SetAttr(get(),
                            name.get(),
                            object(std::forward<Value>(value)).get()));
  }
  template<class Value>
  void set_attr(CStrView name, Value&& value) const {
    ensure(PyObject_SetAttrString(get(),
                                  name.c_str(),
                                  object(std::forward<Value>(value)).get()));
  }
  /// @}

  /// Delete the object attribute, similar to `del obj.attr`.
  /// @{
  void del_attr(const Object& name) const {
    ensure(PyObject_DelAttr(get(), name.get()));
  }
  void del_attr(CStrView name) const {
    ensure(PyObject_DelAttrString(get(), name.c_str()));
  }
  /// @}

  /// Access the item with the given key, similar to `obj[key]`.
  /// @{
  auto at(const Object& key) const -> Object {
    return steal(PyObject_GetItem(get(), key.get()));
  }
  template<class Value>
  void set_at(const Object& key, Value&& value) const {
    ensure(PyObject_SetItem(get(),
                            key.get(),
                            object(std::forward<Value>(value)).get()));
  }
  auto operator[](const Object& key) const -> ItemAt<Object, Object> {
    return {*this, key};
  }
  /// @}

  /// Delete the item with the given key, similar to `del obj[key]`.
  void del(const Object& key) const {
    ensure(PyObject_DelItem(get(), key.get()));
  }

  /// Invoke the object via the `tp_call` protocol.
  /// @{
  auto tp_call() const -> Object; // defined below.
  auto tp_call(const Tuple& args) const -> Object;
  auto tp_call(const Tuple& args, const Dict& kwargs) const -> Object;
  template<class... Args>
  auto operator()(Args&&... args) const -> Object;
  /// @}

  /// Check if the object represents a true value, similar to `bool(obj)`.
  explicit operator bool() const {
    return ensure(PyObject_IsTrue(get()));
  }

  /// Check if the object represents a false value, similar to `not obj`.
  auto operator!() const -> bool {
    return ensure(PyObject_Not(get()));
  }

  /// Comparison operators.
  /// @{
  friend auto operator==(const Object& a, const Object& b) -> bool {
    return ensure(PyObject_RichCompareBool(a.get(), b.get(), Py_EQ));
  }
  friend auto operator!=(const Object& a, const Object& b) -> bool {
    return ensure(PyObject_RichCompareBool(a.get(), b.get(), Py_NE));
  }
  friend auto operator<(const Object& a, const Object& b) -> bool {
    return ensure(PyObject_RichCompareBool(a.get(), b.get(), Py_LT));
  }
  friend auto operator<=(const Object& a, const Object& b) -> bool {
    return ensure(PyObject_RichCompareBool(a.get(), b.get(), Py_LE));
  }
  friend auto operator>(const Object& a, const Object& b) -> bool {
    return ensure(PyObject_RichCompareBool(a.get(), b.get(), Py_GT));
  }
  friend auto operator>=(const Object& a, const Object& b) -> bool {
    return ensure(PyObject_RichCompareBool(a.get(), b.get(), Py_GE));
  }
  /// @}

  /// Arithmetic operations.
  /// @{
  friend auto operator+(const Object& a) -> Object {
    return steal(PyNumber_Positive(a.get()));
  }
  friend auto operator-(const Object& a) -> Object {
    return steal(PyNumber_Negative(a.get()));
  }
  friend auto operator+(const Object& a, const Object& b) -> Object {
    return steal(PyNumber_Add(a.get(), b.get()));
  }
  friend auto operator-(const Object& a, const Object& b) -> Object {
    return steal(PyNumber_Subtract(a.get(), b.get()));
  }
  friend auto operator*(const Object& a, const Object& b) -> Object {
    return steal(PyNumber_Multiply(a.get(), b.get()));
  }
  friend auto operator/(const Object& a, const Object& b) -> Object {
    return steal(PyNumber_TrueDivide(a.get(), b.get()));
  }
  friend auto operator%(const Object& a, const Object& b) -> Object {
    return steal(PyNumber_Remainder(a.get(), b.get()));
  }
  friend auto operator+=(Object& a, const Object& b) -> Object& {
    a = steal(PyNumber_InPlaceAdd(a.get(), b.get()));
    return a;
  }
  friend auto operator-=(Object& a, const Object& b) -> Object& {
    a = steal(PyNumber_InPlaceSubtract(a.get(), b.get()));
    return a;
  }
  friend auto operator*=(Object& a, const Object& b) -> Object& {
    a = steal(PyNumber_InPlaceMultiply(a.get(), b.get()));
    return a;
  }
  friend auto operator/=(Object& a, const Object& b) -> Object& {
    a = steal(PyNumber_InPlaceTrueDivide(a.get(), b.get()));
    return a;
  }
  friend auto operator%=(Object& a, const Object& b) -> Object& {
    a = steal(PyNumber_InPlaceRemainder(a.get(), b.get()));
    return a;
  }
  /// @}

  /// Bitwise operations.
  /// @{
  friend auto operator~(const Object& a) -> Object {
    return steal(PyNumber_Invert(a.get()));
  }
  friend auto operator&(const Object& a, const Object& b) -> Object {
    return steal(PyNumber_And(a.get(), b.get()));
  }
  friend auto operator|(const Object& a, const Object& b) -> Object {
    return steal(PyNumber_Or(a.get(), b.get()));
  }
  friend auto operator^(const Object& a, const Object& b) -> Object {
    return steal(PyNumber_Xor(a.get(), b.get()));
  }
  friend auto operator<<(const Object& a, const Object& b) -> Object {
    return steal(PyNumber_Lshift(a.get(), b.get()));
  }
  friend auto operator>>(const Object& a, const Object& b) -> Object {
    return steal(PyNumber_Rshift(a.get(), b.get()));
  }
  friend auto operator&=(Object& a, const Object& b) -> Object& {
    a = steal(PyNumber_InPlaceAnd(a.get(), b.get()));
    return a;
  }
  friend auto operator|=(Object& a, const Object& b) -> Object& {
    a = steal(PyNumber_InPlaceOr(a.get(), b.get()));
    return a;
  }
  friend auto operator^=(Object& a, const Object& b) -> Object& {
    a = steal(PyNumber_InPlaceXor(a.get(), b.get()));
    return a;
  }
  friend auto operator<<=(Object& a, const Object& b) -> Object& {
    a = steal(PyNumber_InPlaceLshift(a.get(), b.get()));
    return a;
  }
  friend auto operator>>=(Object& a, const Object& b) -> Object& {
    a = steal(PyNumber_InPlaceRshift(a.get(), b.get()));
    return a;
  }
  /// @}

}; // class Object

/// Check if the object is an instance of the given type.
template<std::derived_from<Object>... Derived>
auto isinstance(const Object& obj) -> bool {
  return (... || Derived::isinstance(obj));
}

/// Length of the object, similar to `len(obj)`.
inline auto len(const Object& obj) -> size_t {
  return ensure<size_t>(PyObject_Length(obj.get()));
}

/// Hash the object, similar to `hash(obj)`.
inline auto hash(const Object& obj) -> size_t {
  return ensure<size_t>(PyObject_Hash(obj.get()));
}

/// Type of the object, similar to `type(obj)`.
inline auto type(const Object& obj) -> Object {
  return Object{PyObject_Type(obj.get())};
}

/// Absolute value of the object, similar to `abs(obj)`.
inline auto abs(const Object& obj) -> Object {
  return steal(PyNumber_Absolute(obj.get()));
}

/// Matrix-multiplication of the objects, similar to `a @ b`.
/// @{
inline auto matmul(const Object& a, const Object& b) -> Object {
  return steal(PyNumber_MatrixMultiply(a.get(), b.get()));
}
inline auto matmul_inplace(Object& a, const Object& b) -> Object& {
  a = steal(PyNumber_InPlaceMatrixMultiply(a.get(), b.get()));
  return a;
}
/// @}

/// Floor division of the objects, similar to `a // b`.
/// @{
inline auto floordiv(const Object& a, const Object& b) -> Object {
  return steal(PyNumber_FloorDivide(a.get(), b.get()));
}
inline auto floordiv_inplace(Object& a, const Object& b) -> Object& {
  a = steal(PyNumber_InPlaceFloorDivide(a.get(), b.get()));
  return a;
}
/// @}

/// Power of the objects, similar to `a ** b`.
/// @{
inline auto pow(const Object& a, const Object& b) -> Object {
  return steal(PyNumber_Power(a.get(), b.get(), Py_None));
}
inline auto pow_inplace(Object& a, const Object& b) -> Object& {
  a = steal(PyNumber_InPlacePower(a.get(), b.get(), Py_None));
  return a;
}
/// @}

template<std::derived_from<Object> Derived>
struct impl::Converter<Derived> final {
  static auto object(const Derived& obj) -> Object {
    return obj; // Already a Python object.
  }
  static auto extract(const Object& obj) -> Derived {
    return expect<Derived>(obj);
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// `None` literal.
inline auto None() -> Object {
  return steal(Py_NewRef(Py_None));
}

template<class Value>
struct impl::Converter<std::optional<Value>> final {
  template<class Arg>
    requires std::same_as<std::optional<Value>, std::remove_cvref_t<Arg>>
  static auto object(Arg&& value) -> Object {
    return value ? py::object(*std::forward<Arg>(value)) : None();
  }
  static auto extract(const Object& obj) -> std::optional<Value> {
    return obj.is(None()) ? std::nullopt : extract<Value>(obj);
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python boolean object reference.
class Bool final : public Object {
public:

  /// Check if the object is a subclass of `Bool`.
  static auto isinstance(const Object& obj) -> bool {
    return ensure(PyBool_Check(obj.get()));
  }

  /// Construct a new boolean object from C++ boolean.
  explicit Bool(bool value = false)
      : Object{PyBool_FromLong(static_cast<long>(value))} {}

  /// Construct a new boolean object from Python object, similar to `bool(obj)`.
  explicit Bool(const Object& obj) : Bool{static_cast<bool>(obj)} {}

  /// Get the C++ boolean value.
  auto val() const -> bool {
    return ensure(PyObject_IsTrue(get()));
  }

}; // class Bool

template<>
struct impl::Converter<bool> final {
  static auto object(bool value) -> Object {
    return Bool{value};
  }
  static auto extract(const Object& obj) -> bool {
    return static_cast<bool>(obj);
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python integer object reference.
class Int final : public Object {
public:

  /// Check if the object is a subclass of `Int`.
  static auto isinstance(const Object& obj) -> bool {
    return ensure(PyLong_Check(obj.get()));
  }

  /// Construct a new integer object from C++ integer.
  /// @{
  explicit Int() : Int{0} {}
  explicit Int(std::integral auto value)
      : Object{PyLong_FromLongLong(static_cast<long long>(value))} {}
  /// @}

  /// Construct a new integer object from Python object, similar to `int(obj)`.
  explicit Int(const Object& obj) : Object{PyNumber_Long(obj.get())} {}

  /// Get the C++ integer value.
  auto val() const -> long long {
    const auto result = PyLong_AsLongLong(get());
    if (result == -1 && PyErr_Occurred() != nullptr) raise();
    return result;
  }

}; // class Int

template<std::integral Value>
struct impl::Converter<Value> final {
  static auto object(Value value) -> Object {
    return Int{value};
  }
  static auto extract(const Object& obj) -> Value {
    return static_cast<Value>(expect<Int>(obj).val());
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python float object reference.
class Float final : public Object {
public:

  /// Check if the object is a subclass of `Float`.
  static auto isinstance(const Object& obj) -> bool {
    return ensure(PyFloat_Check(obj.get()));
  }

  /// Construct a new float object from C++ floating-point value.
  /// @{
  explicit Float() : Float{0.0} {}
  explicit Float(std::floating_point auto value)
      : Object{PyFloat_FromDouble(static_cast<double>(value))} {}
  /// @}

  /// Construct a new float object from Python object, similar to `float(obj)`.
  explicit Float(const Object& obj) : Object{PyNumber_Float(obj.get())} {}

  /// Get the C++ floating-point value.
  auto val() const -> double {
    const auto result = PyFloat_AsDouble(get());
    if (is_error_set()) raise();
    return result;
  }

}; // class Float

template<std::floating_point Value>
struct impl::Converter<Value> final {
  static auto object(Value value) -> Object {
    return Float{value};
  }
  static auto extract(const Object& obj) -> Value {
    return static_cast<Value>(expect<Float>(obj).val());
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python iterator reference.
class Iterator final : public Object {
public:

  /// Check if the object implements the iterator protocol.
  static auto isinstance(const Object& obj) -> bool {
    return ensure(PyIter_Check(obj.get()));
  }

  /// Get the next item, similar to `next(iterator)`.
  auto next() const -> std::optional<Object> {
    if (auto* const item = PyIter_Next(get()); item != nullptr) {
      return steal(item);
    }
    if (is_error_set()) raise();
    return std::nullopt;
  }

}; // class Iterator

/// Iterate over the iterable object.
inline auto iter(const Object& iterable) -> Iterator {
  return steal<Iterator>(PyObject_GetIter(iterable.get()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python sequence reference.
class Sequence : public Object {
public:

  using Object::at;
  using Object::del;
  using Object::set_at;
  using Object::operator[];

  /// Check if the object implements the sequence protocol.
  static auto isinstance(const Object& obj) -> bool {
    return ensure(PySequence_Check(obj.get()));
  }

  /// Access an item in the sequence.
  /// @{
  auto at(size_t index) const -> Object {
    return steal(PySequence_GetItem(get(), to_signed(index)));
  }
  template<class Value>
  void set_at(size_t index, Value&& value) const {
    ensure(PySequence_SetItem(get(),
                              to_signed(index),
                              object(std::forward<Value>(value)).get()));
  }
  auto operator[](size_t index) const -> ItemAt<Sequence, size_t> {
    return {*this, index};
  }
  /// @}

  /// Access a slice of the sequence.
  /// @{
  auto at(pair_of_t<size_t> slice) const -> Sequence {
    return steal<Sequence>(PySequence_GetSlice(get(),
                                               to_signed(slice.first),
                                               to_signed(slice.second)));
  }
  template<class Value>
  void set_at(pair_of_t<size_t> slice, Value&& values) const {
    ensure(PySequence_SetSlice(get(),
                               to_signed(slice.first),
                               to_signed(slice.second),
                               object(std::forward<Value>(values)).get()));
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
  template<class Value>
  auto count(Value&& value) const -> size_t {
    return ensure<size_t>(
        PySequence_Count(get(), object(std::forward<Value>(value)).get()));
  }

  /// Check that the sequence contains the given item.
  template<class Value>
  auto contains(Value&& value) const -> bool {
    return ensure(
        PySequence_Contains(get(), object(std::forward<Value>(value)).get()));
  }

  /// Find the index of the item in the sequence.
  template<class Value>
  auto index(Value&& value) const -> size_t {
    return ensure<size_t>(
        PySequence_Index(get(), object(std::forward<Value>(value)).get()));
  }

  /// Repeat the sequence.
  /// @{
  friend auto operator*(size_t n, const Sequence& a) -> Sequence {
    return a * n;
  }
  friend auto operator*(const Sequence& a, size_t n) -> Sequence {
    return steal<Sequence>(PySequence_Repeat(a.get(), to_signed(n)));
  }
  friend auto operator*=(Sequence& a, size_t n) -> Sequence& {
    a = steal<Sequence>(PySequence_InPlaceRepeat(a.get(), to_signed(n)));
    return a;
  }
  /// @}

protected:

  /// Construct a new reference to the existing sequence object.
  explicit Sequence(PyObject* ptr) : Object{ptr} {
    TIT_ASSERT(isinstance(*this), "Object is not a sequence!");
  }

}; // class Sequence

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python string reference.
class Str final : public Sequence {
public:

  /// Check if the object is a subclass of `Str`.
  static auto isinstance(const Object& obj) -> bool {
    return ensure(PyUnicode_Check(obj.get()));
  }

  /// Construct a string object from C++ string.
  /// @{
  explicit Str() : Str{""} {}
  explicit Str(const std::string_view str)
      : Sequence{
            // NOLINTNEXTLINE(*-suspicious-stringview-data-usage)
            PyUnicode_FromStringAndSize(str.data(), to_signed(str.size()))} {}
  /// @}

  /// Construct a string object from Python object, similar to `str(obj)`.
  explicit Str(const Object& obj) : Sequence{PyObject_Str(obj.get())} {}

  /// Get the C++ string.
  auto val() const -> CStrView {
    Py_ssize_t size = 0;
    const auto* const ptr = PyUnicode_AsUTF8AndSize(get(), &size);
    if (ptr == nullptr) raise();
    return CStrView{ptr, to_unsigned(size)};
  }

}; // class Str

/// String representation, similar to `str(obj)`.
inline auto str(const Object& obj) -> std::string {
  return std::string{Str{obj}.val()};
}

/// Object representation, similar to `repr(obj)`.
inline auto repr(const Object& obj) -> std::string {
  return std::string{steal<Str>(PyObject_Repr(obj.get())).val()};
}

// Converter specialization for strings.
template<str_like Value>
struct impl::Converter<Value> final {
  static auto object(const Value& arg) -> Str {
    return Str{arg};
  }
  static auto extract(const Object& obj) -> std::decay_t<Value> {
    return Value{expect<Str>(obj).val()};
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python tuple.
class Tuple final : public Sequence {
public:

  /// Check if the object is a subclass of `Tuple`.
  static auto isinstance(const Object& obj) -> bool {
    return ensure(PyTuple_Check(obj.get()));
  }

  /// Construct a new empty tuple.
  explicit Tuple() : Sequence{PyTuple_New(0)} {}

  /// Construct a tuple object from Python object, similar to `tuple(obj)`.
  explicit Tuple(const Object& iterable)
      // Note: despite the name, `PySequence_Tuple` actually accepts iterables.
      : Sequence{PySequence_Tuple(iterable.get())} {}

}; // class Tuple

/// Construct a new tuple object containing the given items.
template<class... Values>
auto make_tuple(Values&&... values) -> Tuple {
  // We cannot assign the items with `operator[]` because it would call
  // `PySequence_SetItem`, which will trigger a `TypeError`:
  // "'tuple' object does not support item assignment".
  static constexpr auto Size = sizeof...(Values);
  return [&values...]<Py_ssize_t... Indices>(
             std::integer_sequence<Py_ssize_t, Indices...> /*indices*/) {
    auto result = steal<Tuple>(PyTuple_New(Size));
    (ensure(PyTuple_SetItem(result.get(),
                            Indices,
                            object(std::forward<Values>(values)).release())),
     ...);
    return result;
  }(std::make_integer_sequence<Py_ssize_t, sizeof...(Values)>{});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python list reference.
class List final : public Sequence {
public:

  /// Check if the object is a subclass of `List`.
  static auto isinstance(const Object& obj) -> bool {
    return ensure(PyList_Check(obj.get()));
  }

  /// Construct a new empty list.
  explicit List() : Sequence{PyList_New(0)} {}

  /// Construct a list object from Python object, similar to `list(obj)`.
  explicit List(const Object& iterable)
      // Note: despite the name, `PySequence_List` actually accepts iterables.
      : Sequence{PySequence_List(iterable.get())} {}

  /// Insert an item to the list.
  template<class Value>
  void insert(size_t index, Value&& value) const {
    ensure(PyList_Insert(get(),
                         to_signed(index),
                         object(std::forward<Value>(value)).get()));
  }

  /// Append an item to the list.
  template<class Value>
  void append(Value&& value) const {
    ensure(PyList_Append(get(), object(std::forward<Value>(value)).get()));
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

/// Construct a new list object containing the given items.
template<class... Values>
auto make_list(Values&&... values) -> List {
  // We cannot assign the items with `operator[]` because it would call
  // `PySequence_SetItem`, which cannot be used to assign to a partially
  // initialized list.
  static constexpr auto Size = sizeof...(Values);
  return [&values...]<Py_ssize_t... Indices>(
             std::integer_sequence<Py_ssize_t, Indices...> /*indices*/) {
    auto result = steal<List>(PyList_New(Size));
    (ensure(PyList_SetItem(result.get(),
                           Indices,
                           object(std::forward<Values>(values)).release())),
     ...);
    return result;
  }(std::make_integer_sequence<Py_ssize_t, Size>{});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python object that implements the mapping protocol.
class Mapping : public Object {
public:

  using Object::at;
  using Object::del;
  using Object::set_at;
  using Object::operator[];

  /// Check if the object implements the mapping protocol.
  static auto isinstance(const Object& obj) -> bool {
    return ensure(PyMapping_Check(obj.get()));
  }

  /// Check if the mapping contains the given key.
  /// @{
  auto has_key(const Object& key) const -> bool {
    return ensure(PyMapping_HasKey(get(), key.get()));
  }
  auto has_key(CStrView key) const -> bool {
    return ensure(PyMapping_HasKeyString(get(), key.c_str()));
  }
  /// @}

  /// Access the item with the given key.
  /// @{
  auto at(CStrView key) const -> Object {
    return steal(PyMapping_GetItemString(get(), key.c_str()));
  }
  template<class Value>
  void set_at(CStrView key, Value&& value) const {
    ensure(PyMapping_SetItemString(get(),
                                   key.c_str(),
                                   object(std::forward<Value>(value)).get()));
  }
  auto operator[](CStrView key) const {
    return ItemAt<Mapping, std::string>{*this, std::string{key}};
  }
  /// @}

  /// Delete the item with the given key.
  void del(CStrView key) const {
    ensure(PyMapping_DelItemString(get(), key.c_str()));
  }

  /// Keys of the mapping.
  auto keys() const -> List {
    return steal<List>(PyMapping_Keys(get()));
  }

  /// Values of the mapping.
  auto values() const -> List {
    return steal<List>(PyMapping_Values(get()));
  }

  /// Items of the mapping.
  auto items() const -> List {
    return steal<List>(PyMapping_Items(get()));
  }

protected:

  /// Construct a new reference to the existing mapping object.
  explicit Mapping(PyObject* ptr) : Object{ptr} {
    TIT_ASSERT(isinstance(*this), "Object is not a mapping!");
  }

}; // class Mapping

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python dictionary.
class Dict final : public Mapping {
public:

  /// Check if the object is a subclass of `Dict`.
  static auto isinstance(const Object& obj) -> bool {
    return ensure(PyDict_Check(obj.get()));
  }

  /// Construct a new empty dictionary.
  explicit Dict() : Mapping{PyDict_New()} {}

  /// Construct a dictionary object from Python object, similar to `dict(obj)`.
  explicit Dict(const Object& mapping_or_iterable) : Dict{} {
    update(mapping_or_iterable);
  }

  /// Clear the dict.
  void clear() const {
    PyDict_Clear(get()); // never fails.
  }

  /// Update the dict with the other mapping or iterable of key-value pairs.
  void update(const Object& mapping_or_iterable) const {
    auto self = *this;
    self |= mapping_or_iterable;
    TIT_ASSERT(self.is(*this), "Dictionary is immutable?");
  }

  /// Iterate over the dictionary.
  void for_each(std::invocable<const Object&, const Object&> auto func) const {
    Py_ssize_t pos = 0;
    PyObject* key = nullptr;
    PyObject* value = nullptr;
    while (ensure(PyDict_Next(get(), &pos, &key, &value))) {
      std::invoke(func, borrow(key), borrow(value));
    }
  }

}; // class Dict

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python set.
class Set final : public Object {
public:

  /// Check if the object is a subclass of `Set`.
  static auto isinstance(const Object& obj) -> bool {
    return ensure(PySet_Check(obj.get()));
  }

  /// Construct a new empty set object.
  explicit Set() : Object{PySet_New(nullptr)} {}

  /// Construct a set object from Python object, similar to `set(obj)`.
  explicit Set(const Object& iterable) : Object{PySet_New(iterable.get())} {}

  /// Clear the set.
  void clear() const {
    PySet_Clear(get()); // never fails.
  }

  /// Check if the set contains the given item.
  template<class Value>
  auto has(Value&& value) const -> bool {
    return ensure(
        PySet_Contains(get(), object(std::forward<Value>(value)).get()));
  }

  /// Add the item to the set.
  template<class Value>
  void add(Value&& value) const {
    ensure(PySet_Add(get(), object(std::forward<Value>(value)).get()));
  }

  /// Remove the item from the set.
  template<class Value>
  void discard(Value&& value) const {
    ensure(PySet_Discard(get(), object(std::forward<Value>(value)).get()));
  }

  /// Pop an item from the set.
  auto pop() const -> Object {
    return steal(PySet_Pop(get()));
  }

}; // class Set

/// Construct a new set object containing the given items.
template<class... Values>
auto make_set(Values&&... values) -> Set {
  Set result;
  (result.add(std::forward<Values>(values)), ...);
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Call keyword argument.
struct Kwarg final {
  CStrView name;
  Object value;
};

/// Make a keyword argument.
template<class Value>
auto kwarg(CStrView name, Value&& value) -> Kwarg {
  return Kwarg{name, object(std::forward<Value>(value))};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<std::derived_from<Object> Derived>
auto steal(PyObject* obj) -> Derived {
  return expect<Derived>(Object{obj});
}

template<std::derived_from<Object> Derived>
auto borrow(PyObject* obj) -> Derived {
  return expect<Derived>(Object{Py_XNewRef(obj)});
}

template<class Value>
auto object(Value&& value) -> Object {
  return impl::Converter<std::remove_cvref_t<Value>>::object(
      std::forward<Value>(value));
}

template<class Value>
auto extract(const Object& obj) -> decltype(auto) {
  return impl::Converter<std::remove_cvref_t<Value>>::extract(obj);
}

inline auto Object::tp_call() const -> Object {
  return steal(PyObject_CallNoArgs(get()));
}
inline auto Object::tp_call(const Tuple& args) const -> Object {
  return steal(PyObject_CallObject(get(), args.get()));
}
inline auto Object::tp_call(const Tuple& args, const Dict& kwargs) const
    -> Object {
  return steal(PyObject_Call(get(), args.get(), kwargs.get()));
}

template<class... Args>
auto Object::operator()(Args&&... args) const -> Object {
  if constexpr ((std::same_as<std::remove_cvref_t<Args>, Kwarg> || ...)) {
    const Dict kwargs;
    return std::apply(
        [&kwargs, this]<class... PosArgs>(PosArgs&&... pos_args) {
          return this->tp_call(
              py::make_tuple(std::forward<PosArgs>(pos_args)...),
              kwargs);
        },
        std::tuple_cat([&kwargs]<class Arg>(Arg&& arg) {
          if constexpr (std::same_as<std::remove_cvref_t<Arg>, Kwarg>) {
            kwargs[arg.name] = std::move(arg.value);
            return std::tuple{};
          } else {
            return std::tuple{object(std::forward<Arg>(arg))};
          }
        }(std::forward<Args>(args))...));
  } else if constexpr (sizeof...(Args) > 0) {
    return tp_call(py::make_tuple(args...));
  } else {
    return tp_call();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
