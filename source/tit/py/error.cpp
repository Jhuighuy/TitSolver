/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <format>
#include <optional>
#include <string>
#include <utility>

#include "tit/core/checks.hpp"

#include "tit/py/_python.hpp"
#include "tit/py/cast.hpp"
#include "tit/py/error.hpp"
#include "tit/py/module.hpp"
#include "tit/py/object.hpp"
#include "tit/py/type.hpp"

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

auto BaseException::traceback() const -> std::optional<Traceback> {
  if (auto* const result = PyException_GetTraceback(get()); result != nullptr) {
    return steal<Traceback>(result);
  }
  ensure_no_error();
  return std::nullopt;
}

auto BaseException::render() const -> std::string {
  auto result =
      std::format("{}: {}", py::type(*this).fully_qualified_name(), str(*this));
  if (const auto tb = traceback(); tb.has_value()) {
    result = std::format("{}\n\n{}", result, tb->render());
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ErrorException::ErrorException() : message_{error().render()} {}

auto ErrorException::what() const noexcept -> const char* {
  return message_.c_str();
}

[[noreturn]] void raise() {
  throw ErrorException{};
}

[[noreturn]] void raise_type_error(CStrView message) {
  PyErr_SetString(PyExc_TypeError, message.c_str());
  raise();
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
