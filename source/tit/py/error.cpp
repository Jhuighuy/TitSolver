/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <format>
#include <string>
#include <utility>

#include "tit/core/checks.hpp"

#include "tit/py/_python.hpp"
#include "tit/py/error.hpp"
#include "tit/py/module.hpp"
#include "tit/py/object.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Traceback::type() -> Type {
  return borrow(&PyTraceBack_Type);
}

auto Traceback::isinstance(const Object& obj) -> bool {
  return ensure(PyTraceBack_Check(obj.get()));
}

auto Traceback::render() const -> std::string {
  const auto StringIO = import_("io").attr("StringIO");
  const auto stream = StringIO();
  PyTraceBack_Print(get(), stream.get());
  return extract<std::string>(stream.attr("getvalue")());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto BaseException::type() -> Type {
  return borrow<Type>(PyExc_BaseException);
}

auto BaseException::isinstance(const Object& obj) -> bool {
  return ensure(py::type(obj).is_subtype_of(type()));
}

auto BaseException::cause() const -> Optional<Object> {
  return maybe_steal(PyException_GetCause(get()));
}
void BaseException::set_cause(Optional<Object> cause) const {
  PyException_SetCause(get(), cause ? cause.release() : nullptr);
}

auto BaseException::context() const -> Optional<Object> {
  return maybe_steal(PyException_GetContext(get()));
}
void BaseException::set_context(Optional<Object> context) const {
  PyException_SetContext(get(), context ? context.release() : nullptr);
}

auto BaseException::traceback() const -> Optional<Traceback> {
  return maybe_steal<Traceback>(PyException_GetTraceback(get()));
}
void BaseException::set_traceback(const Optional<Traceback>& traceback) const {
  /// @todo As the Python 3.13 docs say, we should pass None to
  ///       `PyException_SetTraceback` in order to clear the traceback. However,
  ///       this seem to be broken in Python 3.11. Thus, we return.
  if (!traceback) return;
  ensure(PyException_SetTraceback(get(), traceback.get()));
}

auto BaseException::render() const -> std::string {
  auto result =
      std::format("{}: {}", py::type(*this).fully_qualified_name(), str(*this));
  if (const auto tb = traceback(); tb) {
    result = std::format("{}\n\n{}", result, expect<Traceback>(tb).render());
  }
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// @todo Use `PyErr_(Get|Set)RaisedException` once we have Python 3.12.
ErrorScope::ErrorScope() noexcept {
  TIT_ASSERT(is_error_set(), "No error was set!");
  PyErr_Fetch(&type_, &value_, &traceback_);
  PyErr_NormalizeException(&type_, &value_, &traceback_);
}

ErrorScope::ErrorScope(ErrorScope&& other) noexcept
    : type_{std::exchange(other.type_, nullptr)},
      value_{std::exchange(other.value_, nullptr)},
      traceback_{std::exchange(other.traceback_, nullptr)} {}

auto ErrorScope::operator=(ErrorScope&& other) noexcept -> ErrorScope& {
  if (this != &other) {
    Py_CLEAR(type_);
    Py_CLEAR(value_);
    Py_CLEAR(traceback_);
    type_ = std::exchange(other.type_, nullptr);
    value_ = std::exchange(other.value_, nullptr);
    traceback_ = std::exchange(other.traceback_, nullptr);
  }
  return *this;
}

ErrorScope::~ErrorScope() noexcept {
  Py_CLEAR(type_);
  Py_CLEAR(value_);
  Py_CLEAR(traceback_);
}

void ErrorScope::restore() noexcept {
  TIT_ASSERT(value_ != nullptr, "Error scope was moved away!");
  PyErr_Restore(type_, value_, traceback_);
  type_ = value_ = traceback_ = nullptr;
}

auto ErrorScope::error() const -> BaseException {
  TIT_ASSERT(value_ != nullptr, "Error scope was moved away!");
  return borrow<BaseException>(value_);
}
void ErrorScope::set_error(BaseException value) noexcept {
  value_ = value.release();
}

void ErrorScope::prefix_message(CStrView prefix) {
  const auto value = borrow<BaseException>(value_);
  const auto message = std::format("{}: {}", prefix, str(value));
  const auto type = borrow<Type>(type_);
  auto new_value = expect<BaseException>(type(message));
  new_value.set_cause(value.cause());
  new_value.set_context(value.context());
  new_value.set_traceback(value.traceback());
  set_error(std::move(new_value));
}

void set_type_error(CStrView message) noexcept {
  PyErr_SetString(PyExc_TypeError, message.c_str());
}
void set_assertion_error(CStrView message) noexcept {
  PyErr_SetString(PyExc_AssertionError, message.c_str());
}
void set_system_error(CStrView message) noexcept {
  PyErr_SetString(PyExc_SystemError, message.c_str());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ErrorException::ErrorException() : message_{error().render()} {}

auto ErrorException::what() const noexcept -> const char* {
  return message_.c_str();
}

[[noreturn]] void raise() {
  throw ErrorException{};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto is_error_set() -> bool {
  return PyErr_Occurred() != nullptr;
}

void clear_error() {
  TIT_ASSERT(is_error_set(), "Cannot clear error that is not set!");
  PyErr_Clear();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
