/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json_fwd.hpp>

#include "tit/core/basic_types.hpp"
#include "tit/core/type.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Property value type.
enum PropType : uint8_t {
  bool_,
  int8,
  uint8,
  int16,
  uint16,
  int32,
  uint32,
  int64,
  uint64,
  float32,
  float64,
  str,
  array,
  record,
  variant
};

/// Convert a property value type to a string.
auto prop_type_to_string(PropType type) -> std::string_view;

/// Construct a property value type from a string.
auto prop_type_from_string(std::string_view type) -> PropType;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Property value specification.
class PropValSpec : public VirtualBase {
public:

  /// Construct a property value specification.
  constexpr PropValSpec() = default;

  /// Construct a property value specification from JSON.
  explicit PropValSpec(const nlohmann::json& json);

  /// Convert the property value specification to JSON.
  virtual auto to_json() const -> nlohmann::json = 0;

  /// Type of the property value.
  virtual auto type() const noexcept -> PropType = 0;

}; // class PropValSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Boolean property value specification.
class BoolPropValSpec final : public PropValSpec {
public:

  /// Construct a boolean property value specification.
  explicit BoolPropValSpec(bool default_value = false);

  /// Construct a boolean property value specification from JSON.
  explicit BoolPropValSpec(const nlohmann::json& json);

  /// Convert the boolean property value specification to JSON.
  auto to_json() const -> nlohmann::json final;

  /// Type of the boolean property value.
  auto type() const noexcept -> PropType final;

  /// Default value of boolean.
  auto default_value() const noexcept -> bool;

private:

  bool default_value_ = false;

}; // class BoolPropValSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Numeric property value specification.
template<class Num>
  requires std::integral<Num> || std::floating_point<Num>
class NumPropValSpec final : public PropValSpec {
public:

  /// Construct a numeric property value specification.
  explicit NumPropValSpec(std::optional<Num> default_value = std::nullopt,
                          std::optional<Num> min_value = std::nullopt,
                          std::optional<Num> max_value = std::nullopt);

  /// Construct a numeric property value specification from JSON.
  explicit NumPropValSpec(const nlohmann::json& json);

  /// Convert the numeric property value specification to JSON.
  auto to_json() const -> nlohmann::json final;

  /// Type of the numeric property value.
  auto type() const noexcept -> PropType final;

  /// Default value of the numeric.
  auto default_value() const noexcept -> std::optional<Num>;

  /// Minimum value of the numeric.
  auto min_value() const noexcept -> std::optional<Num>;

  /// Maximum value of the numeric.
  auto max_value() const noexcept -> std::optional<Num>;

private:

  std::optional<Num> default_value_;
  std::optional<Num> min_value_;
  std::optional<Num> max_value_;

}; // class NumPropValSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// String property value specification.
class StrPropValSpec final : public PropValSpec {
public:

  /// Construct a string property value specification.
  explicit StrPropValSpec(
      std::optional<std::string> default_value = std::nullopt);

  /// Construct a string property value specification from JSON.
  explicit StrPropValSpec(const nlohmann::json& json);

  /// Type of the string property value.
  auto type() const noexcept -> PropType final;

  /// Convert the string property value specification to JSON.
  auto to_json() const -> nlohmann::json final;

  /// Default value of the string.
  auto default_value() const noexcept -> std::optional<std::string_view>;

private:

  std::optional<std::string> default_value_;

}; // class StrPropValSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Array property value specification.
class ArrayPropValSpec final : public PropValSpec {
public:

  /// Construct an array property specification.
  explicit ArrayPropValSpec(std::unique_ptr<PropValSpec> item_spec);

  /// Construct an array property value specification from JSON.
  explicit ArrayPropValSpec(const nlohmann::json& json);

  /// Type of the array property value.
  auto type() const noexcept -> PropType final;

  /// Convert the array property value specification to JSON.
  auto to_json() const -> nlohmann::json final;

  /// Value specification of the item.
  auto item_spec() const noexcept -> const PropValSpec&;

private:

  std::unique_ptr<PropValSpec> item_spec_;

}; // class ArrayPropValSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Composite property field specification.
class FieldSpec final {
public:

  /// Construct a field specification.
  FieldSpec(std::string key,
            std::string name,
            std::string description,
            std::unique_ptr<PropValSpec> value_spec);

  /// Construct a field specification from JSON.
  explicit FieldSpec(const nlohmann::json& json);

  /// Convert the field specification to JSON.
  auto to_json() const -> nlohmann::json;

  /// Key of the record field.
  auto key() const noexcept -> std::string_view;

  /// Human-readable name of the record field.
  auto name() const noexcept -> std::string_view;

  /// Human-readable description of the record field.
  auto description() const noexcept -> std::string_view;

  /// Value specification of the record field.
  auto value_spec() const noexcept -> const PropValSpec&;

private:

  std::string key_;
  std::string name_;
  std::string description_;
  std::unique_ptr<PropValSpec> value_spec_;

}; // class FieldSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Record property value specification.
class RecordPropValSpec final : public PropValSpec {
public:

  /// Construct a record property value specification.
  explicit RecordPropValSpec(std::vector<FieldSpec> fields) noexcept;

  /// Construct a record property value specification from JSON.
  explicit RecordPropValSpec(const nlohmann::json& json);

  /// Convert the record property value specification to JSON.
  auto to_json() const -> nlohmann::json final;

  /// Type of the record property value.
  auto type() const noexcept -> PropType final;

  /// Get the number of fields in the record.
  auto size() const noexcept -> size_t;

  /// Get the record field specification at the given index.
  auto at(size_t index) const noexcept -> const FieldSpec&;

private:

  std::vector<FieldSpec> fields_;

}; // class RecordPropValSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Variant property value specification.
class VariantPropValSpec final : public PropValSpec {
public:

  /// Construct a variant property value specification.
  explicit VariantPropValSpec(
      std::vector<FieldSpec> options,
      std::optional<size_t> default_option = std::nullopt) noexcept;

  /// Construct a variant property value specification from JSON.
  explicit VariantPropValSpec(const nlohmann::json& json);

  /// Convert the variant property value specification to JSON.
  auto to_json() const -> nlohmann::json final;

  /// Type of the variant property value.
  auto type() const noexcept -> PropType final;

  /// Get the number of options in the variant.
  auto size() const noexcept -> size_t;

  /// Get the variant option specification at the given index.
  auto at(size_t index) const noexcept -> const FieldSpec&;

  /// Get the default option of the variant.
  auto default_option() const noexcept -> std::optional<size_t>;

private:

  std::vector<FieldSpec> options_;
  std::optional<size_t> default_option_;

}; // class VariantPropValSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Properies specification.
using PropsSpec = RecordPropValSpec;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
