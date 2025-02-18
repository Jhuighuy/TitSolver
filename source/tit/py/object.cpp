/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <bit>
#include <string>
#include <typeinfo>
#include <utility>

#include "tit/core/checks.hpp"
#include "tit/core/str_utils.hpp"

#include "tit/py/_python.hpp"
#include "tit/py/error.hpp"
#include "tit/py/mapping.hpp"
#include "tit/py/number.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

BaseObject::BaseObject(PyObject* ptr) : ptr_{ptr} {
  TIT_ASSERT(ptr_ != nullptr, "Object is null!");
}

BaseObject::BaseObject(BaseObject&& other) noexcept : ptr_{other.release()} {}

BaseObject::BaseObject(const BaseObject& other) noexcept
    : ptr_{Py_XNewRef(other.ptr_)} {}

auto BaseObject::operator=(BaseObject&& other) noexcept -> BaseObject& {
  if (this != &other) {
    Py_CLEAR(ptr_);
    ptr_ = other.release();
  }
  return *this;
}

auto BaseObject::operator=(const BaseObject& other) -> BaseObject& {
  if (this != &other) {
    Py_CLEAR(ptr_);
    ptr_ = Py_XNewRef(other.ptr_);
  }
  return *this;
}

BaseObject::~BaseObject() noexcept {
  Py_CLEAR(ptr_);
}

auto BaseObject::valid() const noexcept -> bool {
  return ptr_ != nullptr;
}

auto BaseObject::get() const noexcept -> PyObject* {
  TIT_ASSERT(valid(), "Object is null!");
  return ptr_;
}

auto BaseObject::release() noexcept -> PyObject* {
  return std::exchange(ptr_, nullptr);
}

void BaseObject::reset(PyObject* ptr) {
  if (ptr_ == ptr) return;
  Py_CLEAR(ptr_);
  ptr_ = ptr;
  TIT_ASSERT(valid(), "Object is null!");
}

void BaseObject::incref() const noexcept {
  TIT_ASSERT(valid(), "Object is null!");
  Py_INCREF(ptr_);
}

void BaseObject::decref() const noexcept {
  TIT_ASSERT(valid(), "Object is null!");
  Py_DECREF(ptr_);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Object::isinstance(const Object& /*obj*/) -> bool {
  return true;
}

auto Object::is(const Object& other) const -> bool {
  return Py_Is(get(), other.get());
}

auto Object::has_attr(const Object& name) const -> bool {
  return ensure(PyObject_HasAttr(get(), name.get()));
}
auto Object::has_attr(CStrView name) const -> bool {
  return ensure(PyObject_HasAttrString(get(), name.c_str()));
}

auto Object::attr(const Object& name) const -> Object {
  return steal(ensure(PyObject_GetAttr(get(), name.get())));
}
auto Object::attr(CStrView name) const -> Object {
  return steal(ensure(PyObject_GetAttrString(get(), name.c_str())));
}
void Object::set_attr(const Object& name, const Object& value) const {
  ensure(PyObject_SetAttr(get(), name.get(), value.get()));
}
void Object::set_attr(CStrView name, const Object& value) const {
  ensure(PyObject_SetAttrString(get(), name.c_str(), value.get()));
}

void Object::del_attr(const Object& name) const {
  ensure(PyObject_DelAttr(get(), name.get()));
}
void Object::del_attr(CStrView name) const {
  ensure(PyObject_DelAttrString(get(), name.c_str()));
}

auto Object::at(const Object& key) const -> Object {
  return steal(ensure(PyObject_GetItem(get(), key.get())));
}
void Object::set_at(const Object& key, const Object& value) const {
  ensure(PyObject_SetItem(get(), key.get(), value.get()));
}

void Object::del(const Object& key) const {
  ensure(PyObject_DelItem(get(), key.get()));
}

/// @todo Use `PyObject_Vectorcall` once we have Python 3.12.
auto Object::call() const -> Object {
  return steal(ensure(PyObject_CallNoArgs(get())));
}
auto Object::tp_call(const Tuple& posargs) const -> Object {
  return steal(ensure(PyObject_CallObject(get(), posargs.get())));
}
auto Object::tp_call(const Tuple& posargs, const Dict& kwargs) const -> Object {
  return steal(ensure(PyObject_Call(get(), posargs.get(), kwargs.get())));
}
auto Object::call(std::span<const Object> posargs) const -> Object {
  return tp_call(to_tuple(posargs));
}
auto Object::call(std::span<const Object> posargs,
                  std::span<const Kwarg> kwargs) const -> Object {
  const Dict kwargs_dict;
  for (const auto& [name, value] : kwargs) kwargs_dict[name] = value;
  return tp_call(to_tuple(posargs), kwargs_dict);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Object::operator bool() const {
  return ensure(PyObject_IsTrue(get()));
}

auto Object::operator!() const -> bool {
  return ensure(PyObject_Not(get()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto operator==(const Object& a, const Object& b) -> bool {
  return ensure(PyObject_RichCompareBool(a.get(), b.get(), Py_EQ));
}
auto operator!=(const Object& a, const Object& b) -> bool {
  return ensure(PyObject_RichCompareBool(a.get(), b.get(), Py_NE));
}
auto operator<(const Object& a, const Object& b) -> bool {
  return ensure(PyObject_RichCompareBool(a.get(), b.get(), Py_LT));
}
auto operator<=(const Object& a, const Object& b) -> bool {
  return ensure(PyObject_RichCompareBool(a.get(), b.get(), Py_LE));
}
auto operator>(const Object& a, const Object& b) -> bool {
  return ensure(PyObject_RichCompareBool(a.get(), b.get(), Py_GT));
}
auto operator>=(const Object& a, const Object& b) -> bool {
  return ensure(PyObject_RichCompareBool(a.get(), b.get(), Py_GE));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto operator+(const Object& a) -> Object {
  return steal(ensure(PyNumber_Positive(a.get())));
}
auto operator-(const Object& a) -> Object {
  return steal(ensure(PyNumber_Negative(a.get())));
}
auto operator+(const Object& a, const Object& b) -> Object {
  return steal(ensure(PyNumber_Add(a.get(), b.get())));
}
auto operator-(const Object& a, const Object& b) -> Object {
  return steal(ensure(PyNumber_Subtract(a.get(), b.get())));
}
auto operator*(const Object& a, const Object& b) -> Object {
  return steal(ensure(PyNumber_Multiply(a.get(), b.get())));
}
auto operator/(const Object& a, const Object& b) -> Object {
  return steal(ensure(PyNumber_TrueDivide(a.get(), b.get())));
}
auto operator%(const Object& a, const Object& b) -> Object {
  return steal(ensure(PyNumber_Remainder(a.get(), b.get())));
}
auto operator+=(Object& a, const Object& b) -> Object& {
  a = steal(ensure(PyNumber_InPlaceAdd(a.get(), b.get())));
  return a;
}
auto operator-=(Object& a, const Object& b) -> Object& {
  a = steal(ensure(PyNumber_InPlaceSubtract(a.get(), b.get())));
  return a;
}
auto operator*=(Object& a, const Object& b) -> Object& {
  a = steal(ensure(PyNumber_InPlaceMultiply(a.get(), b.get())));
  return a;
}
auto operator/=(Object& a, const Object& b) -> Object& {
  a = steal(ensure(PyNumber_InPlaceTrueDivide(a.get(), b.get())));
  return a;
}
auto operator%=(Object& a, const Object& b) -> Object& {
  a = steal(ensure(PyNumber_InPlaceRemainder(a.get(), b.get())));
  return a;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto operator~(const Object& a) -> Object {
  return steal(ensure(PyNumber_Invert(a.get())));
}
auto operator&(const Object& a, const Object& b) -> Object {
  return steal(ensure(PyNumber_And(a.get(), b.get())));
}
auto operator|(const Object& a, const Object& b) -> Object {
  return steal(ensure(PyNumber_Or(a.get(), b.get())));
}
auto operator^(const Object& a, const Object& b) -> Object {
  return steal(ensure(PyNumber_Xor(a.get(), b.get())));
}
auto operator<<(const Object& a, const Object& b) -> Object {
  return steal(ensure(PyNumber_Lshift(a.get(), b.get())));
}
auto operator>>(const Object& a, const Object& b) -> Object {
  return steal(ensure(PyNumber_Rshift(a.get(), b.get())));
}
auto operator&=(Object& a, const Object& b) -> Object& {
  a = steal(ensure(PyNumber_InPlaceAnd(a.get(), b.get())));
  return a;
}
auto operator|=(Object& a, const Object& b) -> Object& {
  a = steal(ensure(PyNumber_InPlaceOr(a.get(), b.get())));
  return a;
}
auto operator^=(Object& a, const Object& b) -> Object& {
  a = steal(ensure(PyNumber_InPlaceXor(a.get(), b.get())));
  return a;
}
auto operator<<=(Object& a, const Object& b) -> Object& {
  a = steal(ensure(PyNumber_InPlaceLshift(a.get(), b.get())));
  return a;
}
auto operator>>=(Object& a, const Object& b) -> Object& {
  a = steal(ensure(PyNumber_InPlaceRshift(a.get(), b.get())));
  return a;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto len(const Object& obj) -> size_t {
  return ensure<size_t>(PyObject_Length(obj.get()));
}

auto hash(const Object& obj) -> size_t {
  return ensure<size_t>(PyObject_Hash(obj.get()));
}

auto str(const Object& obj) -> std::string {
  return std::string{Str{obj}.val()};
}

auto repr(const Object& obj) -> std::string {
  return std::string{steal<Str>(ensure(PyObject_Repr(obj.get()))).val()};
}

auto abs(const Object& obj) -> Object {
  return steal(ensure(PyNumber_Absolute(obj.get())));
}

auto floordiv(const Object& a, const Object& b) -> Object {
  return steal(ensure(PyNumber_FloorDivide(a.get(), b.get())));
}
auto floordiv_inplace(Object& a, const Object& b) -> Object& {
  a = steal(ensure(PyNumber_InPlaceFloorDivide(a.get(), b.get())));
  return a;
}

auto pow(const Object& a, const Object& b) -> Object {
  return steal(ensure(PyNumber_Power(a.get(), b.get(), Py_None)));
}
auto pow_inplace(Object& a, const Object& b) -> Object& {
  a = steal(ensure(PyNumber_InPlacePower(a.get(), b.get(), Py_None)));
  return a;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto NoneType::isinstance(const Object& obj) -> bool {
  return ensure(Py_IsNone(obj.get()));
}

auto None() -> NoneType {
  return borrow<NoneType>(ensure(Py_None));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Type::type() -> Type {
  return borrow(&PyType_Type);
}

auto Type::isinstance(const Object& obj) -> bool {
  return ensure(PyType_Check(obj.get()));
}

auto Type::get_type() const -> PyTypeObject* {
  return std::bit_cast<PyTypeObject*>(get());
}

/// @todo Use `PyType_GetName` once we have Python 3.11.
auto Type::name() const -> std::string {
  return extract<std::string>(attr("__name__"));
}

/// @todo Use `PyType_GetQualName` once we have Python 3.11.
auto Type::qualified_name() const -> std::string {
  return extract<std::string>(attr("__qualname__"));
}

/// @todo Use `PyType_GetFullyQualifiedName` once we have Python 3.13.
auto Type::fully_qualified_name() const -> std::string {
  const auto mod_name = module_name();
  const auto qual_name = qualified_name();
  return mod_name == "builtins" ? qual_name :
                                  std::format("{}.{}", mod_name, qual_name);
}

/// @todo Use `PyType_GetModuleName` once we have Python 3.13.
auto Type::module_name() const -> std::string {
  return extract<std::string>(attr("__module__"));
}

auto Type::is_subtype_of(const Type& other) const -> bool {
  return ensure(PyType_IsSubtype(get_type(), other.get_type()));
}

auto type(const Object& obj) -> Type {
  return steal<Type>(ensure(PyObject_Type(obj.get())));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto steal(PyObject* ptr) -> Object {
  TIT_ASSERT(ptr != nullptr, "Object is null!");
  return Object{ptr};
}

auto borrow(PyObject* ptr) -> Object {
  TIT_ASSERT(ptr != nullptr, "Object is null!");
  return Object{Py_NewRef(ptr)};
}
auto borrow(PyTypeObject* type) -> Type {
  return borrow<Type>(std::bit_cast<PyObject*>(type));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

const size_t sizeof_PyObject = sizeof(PyObject);
const size_t alignof_PyObject = alignof(PyObject);

[[noreturn]] void raise_unexpected_type_error(
    std::string_view expected_type_name,
    const Object& obj) {
  raise_type_error("expected '{}', got '{}'",
                   expected_type_name,
                   type(obj).fully_qualified_name());
}

auto alloc(const std::type_info& type_info) -> PyObject* {
  return ensure(PyType_GenericAlloc(lookup_type(type_info).get_type(), 1));
}
void dealloc(PyObject* ptr) {
  TIT_ASSERT(ptr != nullptr, "Object must not be null!");
  PyObject_Free(ptr);
}

auto Converter<bool>::object(bool value) -> Object {
  return Bool{value};
}
auto Converter<bool>::extract(const Object& obj) -> bool {
  return expect<Bool>(obj).val();
}

auto Converter<long long>::object(long long value) -> Object {
  return Int{value};
}
auto Converter<long long>::extract(const Object& obj) -> long long {
  return expect<Int>(obj).val();
}

auto Converter<double>::object(double value) -> Object {
  return Float{value};
}
auto Converter<double>::extract(const Object& obj) -> double {
  return expect<Float>(obj).val();
}

auto Converter<std::string_view>::object(std::string_view value) -> Object {
  return Str{value};
}
auto Converter<std::string_view>::extract(const Object& obj) -> CStrView {
  return expect<Str>(obj).val();
}

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
