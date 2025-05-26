/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/type.hpp"
#include "tit/prop/json.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Specification type.
enum class SpecType : uint8_t {
  bool_,
  int_,
  float_,
  str,
  enum_,
  array,
  record,
  variant,
};

/// Convert a specification type to a string.
auto spec_type_to_string(SpecType type) -> std::string_view;

/// Construct a specification type from a string.
auto spec_type_from_string(std::string_view string) -> SpecType;

/// Construct a specification type from JSON.
auto spec_type_from_json(const json::JSON& json) -> SpecType;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Specification.
//

/// Pointer to a specification.
using SpecPtr = std::shared_ptr<class Spec>;

/// Abstract specification.
class Spec : public VirtualBase, public std::enable_shared_from_this<Spec> {
public:

  /// Construct a specification from JSON.
  static auto from_json(json::JSON json) -> SpecPtr;

  /// Construct a specification from string.
  static auto from_string(std::string_view string) -> SpecPtr;

  /// Convert the specification to JSON.
  virtual auto to_json() const -> json::JSON = 0;

  /// Convert the specification to string.
  auto to_string() const -> std::string;

  /// Get the type of the specification.
  virtual auto type() const noexcept -> SpecType = 0;

  /// Get the initial value.
  virtual auto initial_value() const -> json::JSON = 0;

  /// Validate the value against the specification.
  virtual void validate_value(json::JSON value) const = 0;

}; // class Spec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Boolean Specification.
//

/// Pointer to a boolean specification.
using BoolSpecPtr = std::shared_ptr<class BoolSpec>;

/// Boolean specification.
class BoolSpec final : public Spec {
public:

  static auto from_json(json::JSON json) -> BoolSpecPtr;
  auto to_json() const -> json::JSON override;
  auto type() const noexcept -> SpecType override;
  auto initial_value() const -> json::JSON override;
  void validate_value(json::JSON value) const override;

private:

  std::optional<bool> init_;
  std::optional<std::string> true_label_;
  std::optional<std::string> false_label_;

}; // class BoolSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Integer Specification.
//

/// Pointer to an integer specification.
using IntSpecPtr = std::shared_ptr<class IntSpec>;

/// Integer specification.
class IntSpec final : public Spec {
public:

  static auto from_json(json::JSON json) -> IntSpecPtr;
  auto to_json() const -> json::JSON override;
  auto type() const noexcept -> SpecType override;
  auto initial_value() const -> json::JSON override;
  void validate_value(json::JSON value) const override;

private:

  std::optional<int64_t> init_;
  std::optional<int64_t> min_;
  std::optional<int64_t> max_;

}; // class IntSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Floating-point Specification.
//

/// Pointer to a floating-point specification.
using FloatSpecPtr = std::shared_ptr<class FloatSpec>;

/// Floating-point specification.
class FloatSpec final : public Spec {
public:

  static auto from_json(json::JSON json) -> FloatSpecPtr;
  auto to_json() const -> json::JSON override;
  auto type() const noexcept -> SpecType override;
  auto initial_value() const -> json::JSON override;
  void validate_value(json::JSON value) const override;

private:

  std::optional<float64_t> init_;
  std::optional<float64_t> min_;
  std::optional<float64_t> max_;
  std::optional<std::string> unit_;

}; // class FloatSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// String Specification.
//

/// Pointer to a string specification.
using StrSpecPtr = std::shared_ptr<class StrSpec>;

/// String specification.
class StrSpec final : public Spec {
public:

  static auto from_json(json::JSON json) -> StrSpecPtr;
  auto to_json() const -> json::JSON override;
  auto type() const noexcept -> SpecType override;
  auto initial_value() const -> json::JSON override;
  void validate_value(json::JSON value) const override;

private:

  std::optional<std::string> init_;

}; // class StrSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Enumeration Specification.
//

/// Pointer to an enumeration option specification.
using EnumOptionSpecPtr = std::unique_ptr<class EnumOptionSpec>;

/// Enumeration option specification.
class EnumOptionSpec final {
public:

  static auto from_json(json::JSON json) -> EnumOptionSpecPtr;
  auto to_json() const -> json::JSON;
  void validate_value(json::JSON value) const;

  /// Get the option name.
  auto name() const noexcept -> std::string_view {
    return name_;
  }

private:

  std::string name_;
  std::optional<std::string> descr_;

}; // class EnumOptionSpec

/// Pointer to an enumeration specification.
using EnumSpecPtr = std::shared_ptr<class EnumSpec>;

/// Enumeration specification.
class EnumSpec final : public Spec {
public:

  static auto from_json(json::JSON json) -> EnumSpecPtr;
  auto to_json() const -> json::JSON override;
  auto type() const noexcept -> SpecType override;
  auto initial_value() const -> json::JSON override;
  void validate_value(json::JSON value) const override;

private:

  std::vector<EnumOptionSpecPtr> options_;
  std::optional<std::string> init_;

}; // class EnumSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Array Specification.
//

/// Pointer to an array specification.
using ArraySpecPtr = std::shared_ptr<class ArraySpec>;

/// Array specification.
class ArraySpec final : public Spec {
public:

  static auto from_json(json::JSON json) -> ArraySpecPtr;
  auto to_json() const -> json::JSON override;
  auto type() const noexcept -> SpecType override;
  auto initial_value() const -> json::JSON override;
  void validate_value(json::JSON value) const override;

private:

  SpecPtr item_spec_;

}; // class ArraySpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Record Specification.
//

/// Pointer to a record field specification.
using RecordFieldSpecPtr = std::unique_ptr<class RecordFieldSpec>;

/// Record field specification.
class RecordFieldSpec final {
public:

  static auto from_json(json::JSON json) -> RecordFieldSpecPtr;
  auto to_json() const -> json::JSON;
  auto initial_value() const -> json::JSON;
  void validate_value(json::JSON value) const;

  /// Get the field name.
  auto name() const noexcept -> std::string_view {
    return name_;
  }

private:

  std::string name_;
  std::optional<std::string> descr_;
  SpecPtr val_spec_;

}; // class RecordFieldSpec

/// Pointer to a record specification.
using RecordSpecPtr = std::shared_ptr<class RecordSpec>;

/// Record specification.
class RecordSpec final : public Spec {
public:

  static auto from_json(json::JSON json) -> RecordSpecPtr;
  auto to_json() const -> json::JSON override;
  auto type() const noexcept -> SpecType override;
  auto initial_value() const -> json::JSON override;
  void validate_value(json::JSON value) const override;

private:

  std::vector<RecordFieldSpecPtr> fields_;

}; // class RecordSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Variant Specification.
//

/// Pointer to a variant option specification.
using VariantOptionSpecPtr = std::unique_ptr<class VariantOptionSpec>;

/// Variant option specification.
class VariantOptionSpec final {
public:

  static auto from_json(json::JSON json) -> VariantOptionSpecPtr;
  auto to_json() const -> json::JSON;
  auto initial_value() const -> json::JSON;
  void validate_value(json::JSON value) const;

  /// Get the option name.
  auto name() const noexcept -> std::string_view {
    return name_;
  }

private:

  std::string name_;
  std::optional<std::string> descr_;
  SpecPtr val_spec_;

}; // class VariantOptionSpec

/// Pointer to a variant specification.
using VariantSpecPtr = std::shared_ptr<class VariantSpec>;

/// Variant specification.
class VariantSpec final : public Spec {
public:

  static auto from_json(json::JSON json) -> VariantSpecPtr;
  auto to_json() const -> json::JSON override;
  auto type() const noexcept -> SpecType override;
  auto initial_value() const -> json::JSON override;
  void validate_value(json::JSON value) const override;

private:

  std::vector<VariantOptionSpecPtr> options_;
  std::optional<std::string> init_;

}; // class VariantSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
