/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/type.hpp"
#include "tit/core/utils.hpp"
#include "tit/data/json.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Parameter specification type.
enum class ParamSpecType : uint8_t {
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
auto param_spec_type_to_string(ParamSpecType type) -> std::string_view;

/// Construct a specification type from a string.
auto param_spec_type_from_string(std::string_view string) -> ParamSpecType;

/// Construct a specification type from JSON.
auto param_spec_type_from_json(const json::json& json) -> ParamSpecType;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a parameter specification.
using ParamSpecPtr = std::unique_ptr<class ParamSpec>;

/// Abstract parameter specification.
class ParamSpec : public VirtualBase {
public:

  /// Construct a specification from JSON.
  static auto from_json(json::json json) -> ParamSpecPtr;

  /// Construct a specification from string.
  static auto from_string(std::string_view string) -> ParamSpecPtr;

  /// Convert the specification to JSON.
  virtual auto to_json() const -> json::json = 0;

  /// Convert the specification to string.
  auto to_string() const -> std::string;

  /// Get the type of the specification.
  virtual auto type() const noexcept -> ParamSpecType = 0;

  /// Validate the value against the specification.
  virtual void validate(std::string_view value) const = 0;

  /// Get the parameter name.
  auto name() const noexcept -> std::optional<std::string_view> {
    return name_.transform(to<std::string_view>);
  }

  /// Get the parameter label.
  auto label() const noexcept -> std::optional<std::string_view> {
    return label_.transform(to<std::string_view>);
  }

private:

  std::optional<std::string> name_;
  std::optional<std::string> label_;

}; // class ParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a boolean parameter specification.
using BoolParamSpecPtr = std::unique_ptr<class BoolParamSpec>;

/// Boolean parameter specification.
class BoolParamSpec final : public ParamSpec {
public:

  static auto from_json(json::json& json) -> BoolParamSpecPtr;
  auto to_json() const -> json::json override;
  auto type() const noexcept -> ParamSpecType override;
  void validate(std::string_view value) const override;

  /// Get the default value.
  auto default_value() const noexcept -> std::optional<bool> {
    return default_;
  }

  /// Get the true label.
  auto true_label() const noexcept -> std::optional<std::string_view> {
    return true_label_.transform(to<std::string_view>);
  }

  /// Get the false label.
  auto false_label() const noexcept -> std::optional<std::string_view> {
    return false_label_.transform(to<std::string_view>);
  }

private:

  std::optional<bool> default_;
  std::optional<std::string> true_label_;
  std::optional<std::string> false_label_;

}; // class BoolParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to an integer parameter specification.
using IntParamSpecPtr = std::unique_ptr<class IntParamSpec>;

/// Integer parameter specification.
class IntParamSpec final : public ParamSpec {
public:

  static auto from_json(json::json& json) -> IntParamSpecPtr;
  auto to_json() const -> json::json override;
  auto type() const noexcept -> ParamSpecType override;
  void validate(std::string_view value) const override;

  /// Get the default value.
  auto default_value() const noexcept -> std::optional<int64_t> {
    return default_;
  }

  /// Get the minimum value.
  auto min_value() const noexcept -> std::optional<int64_t> {
    return min_;
  }

  /// Get the maximum value.
  auto max_value() const noexcept -> std::optional<int64_t> {
    return max_;
  }

private:

  std::optional<int64_t> default_;
  std::optional<int64_t> min_;
  std::optional<int64_t> max_;

}; // class IntParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a floating-point parameter specification.
using FloatParamSpecPtr = std::unique_ptr<class FloatParamSpec>;

/// Floating-point parameter specification.
class FloatParamSpec final : public ParamSpec {
public:

  static auto from_json(json::json& json) -> FloatParamSpecPtr;
  auto to_json() const -> json::json override;
  auto type() const noexcept -> ParamSpecType override;
  void validate(std::string_view value) const override;

  /// Get the default value.
  auto default_value() const noexcept -> std::optional<float64_t> {
    return default_;
  }

  /// Get the minimum value.
  auto min_value() const noexcept -> std::optional<float64_t> {
    return min_;
  }

  /// Get the maximum value.
  auto max_value() const noexcept -> std::optional<float64_t> {
    return max_;
  }

  /// Get the unit.
  auto unit() const noexcept -> std::optional<std::string_view> {
    return unit_.transform(to<std::string_view>);
  }

private:

  std::optional<float64_t> default_;
  std::optional<float64_t> min_;
  std::optional<float64_t> max_;
  std::optional<std::string> unit_;

}; // class FloatParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a string parameter specification.
using StrParamSpecPtr = std::unique_ptr<class StrParamSpec>;

/// String parameter specification.
class StrParamSpec final : public ParamSpec {
public:

  static auto from_json(json::json& json) -> StrParamSpecPtr;
  auto to_json() const -> json::json override;
  auto type() const noexcept -> ParamSpecType override;
  void validate(std::string_view value) const override;

  /// Get the default value.
  auto default_value() const noexcept -> std::optional<std::string_view> {
    return default_.transform(to<std::string_view>);
  }

private:

  std::optional<std::string> default_;

}; // class StrParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to an enumeration parameter specification.
using EnumParamSpecPtr = std::unique_ptr<class EnumParamSpec>;

/// Enumeration parameter specification.
class EnumParamSpec final : public ParamSpec {
public:

  /// Option specification.
  struct OptionSpec final {
    std::string name;                 ///< Option name.
    std::optional<std::string> label; ///< Option label.
  };

  static auto from_json(json::json& json) -> EnumParamSpecPtr;
  auto to_json() const -> json::json override;
  auto type() const noexcept -> ParamSpecType override;
  void validate(std::string_view value) const override;

  /// Get the options.
  auto options() const noexcept -> std::span<const OptionSpec> {
    return options_;
  }

  /// Get the default value.
  auto default_value() const noexcept -> std::optional<std::string_view> {
    return default_;
  }

private:

  std::vector<OptionSpec> options_;
  std::optional<std::string> default_;

}; // class EnumParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to an array parameter specification.
using ArrayParamSpecPtr = std::unique_ptr<class ArrayParamSpec>;

/// Array parameter specification.
class ArrayParamSpec final : public ParamSpec {
public:

  static auto from_json(json::json& json) -> ArrayParamSpecPtr;
  auto to_json() const -> json::json override;
  auto type() const noexcept -> ParamSpecType override;
  void validate(std::string_view value) const override;

  /// Get the item specification.
  auto item_spec() const noexcept -> const ParamSpecPtr& {
    return item_spec_;
  }

private:

  ParamSpecPtr item_spec_;

}; // class ArrayParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a record parameter specification.
using RecordParamSpecPtr = std::unique_ptr<class RecordParamSpec>;

/// Record parameter specification.
class RecordParamSpec final : public ParamSpec {
public:

  static auto from_json(json::json& json) -> RecordParamSpecPtr;
  auto to_json() const -> json::json override;
  auto type() const noexcept -> ParamSpecType override;
  void validate(std::string_view value) const override;

  /// Get the fields.
  auto fields() const noexcept -> std::span<const ParamSpecPtr> {
    return fields_;
  }

private:

  std::vector<ParamSpecPtr> fields_;

}; // class RecordParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a variant parameter specification.
using VariantParamSpecPtr = std::unique_ptr<class VariantParamSpec>;

/// Variant parameter specification.
class VariantParamSpec final : public ParamSpec {
public:

  static auto from_json(json::json& json) -> VariantParamSpecPtr;
  auto to_json() const -> json::json override;
  auto type() const noexcept -> ParamSpecType override;
  void validate(std::string_view value) const override;

  /// Get the options.
  auto options() const noexcept -> std::span<const ParamSpecPtr> {
    return options_;
  }

  /// Get the default value.
  auto default_value() const noexcept -> std::optional<std::string_view> {
    return default_.transform(to<std::string_view>);
  }

private:

  std::vector<ParamSpecPtr> options_;
  std::optional<std::string> default_;

}; // class VariantParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
