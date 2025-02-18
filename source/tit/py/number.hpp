/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/py/object.hpp"

namespace tit::py {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python boolean object reference.
class Bool final : public Object {
public:

  /// Get the type object of the `Bool`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `Bool`.
  static auto isinstance(const Object& obj) -> bool;

  /// Construct a new boolean object from C++ boolean.
  explicit Bool(bool value = false);

  /// Construct a new boolean object from Python object, similar to `bool(obj)`.
  explicit Bool(const Object& obj);

  /// Get the C++ boolean value.
  auto val() const -> bool;

}; // class Bool

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python integer object reference.
class Int final : public Object {
public:

  /// Get the type object of the `Int`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `Int`.
  static auto isinstance(const Object& obj) -> bool;

  /// Construct a new integer object from C++ integer.
  /// @{
  explicit Int(long long value = 0);
  explicit Int(std::integral auto value) : Int{static_cast<long long>(value)} {}
  /// @}

  /// Construct a new integer object from Python object, similar to `int(obj)`.
  explicit Int(const Object& obj);

  /// Get the C++ integer value.
  auto val() const -> long long;

}; // class Int

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python float object reference.
class Float final : public Object {
public:

  /// Get the type object of the `Float`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `Float`.
  static auto isinstance(const Object& obj) -> bool;

  /// Construct a new float object from C++ floating-point value.
  /// @{
  explicit Float(double value = 0.0);
  explicit Float(std::floating_point auto value)
      : Float{static_cast<double>(value)} {}
  /// @}

  /// Construct a new float object from Python object, similar to `float(obj)`.
  explicit Float(const Object& obj);

  /// Get the C++ floating-point value.
  auto val() const -> double;

}; // class Float

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
