/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "tit/core/basic_types.hpp"
#include "tit/core/type.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Property type.
enum class PropType : uint8_t {
  bool_,
  int_,
  float_,
  string,
  record,
  variant,
};

/// Convert a property type to a string.
auto prop_type_to_string(PropType type) -> std::string_view;

/// Construct a property type from a string.
auto prop_type_from_string(std::string_view type) -> PropType;

/// Construct a property type from JSON.
auto prop_type_from_json(const nlohmann::json& json) -> PropType;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a property specification.
using PropSpecPtr = std::unique_ptr<class PropSpec>;

/// Abstract property specification.
class PropSpec : public VirtualBase {
public:

  /// Construct a property specification from JSON.
  static auto from_json(const nlohmann::json& json) -> PropSpecPtr;

  /// Convert the property specification to JSON.
  virtual auto to_json() const -> nlohmann::json = 0;

  /// Get the type of the property.
  constexpr virtual auto type() const noexcept -> PropType = 0;

  /// Get the name of the property (field of a record or option of a variant).
  constexpr auto name() const noexcept -> std::string_view {
    return name_;
  }

  /// Get the description of the property.
  constexpr auto description() const noexcept -> std::string_view {
    return description_;
  }

protected:

  /// Construct a property specification.
  constexpr PropSpec(std::string name, std::string description)
      : name_{std::move(name)}, description_{std::move(description)} {}

  /// Construct a property specification from JSON.
  explicit PropSpec(const nlohmann::json& json);

private:

  std::string name_;
  std::string description_;

}; // class PropSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Boolean property specification.
class BoolPropSpec final : public PropSpec {
public:

  /// Construct a boolean property specification.
  constexpr BoolPropSpec(std::string name,
                         std::string description,
                         std::optional<bool> default_value = {})
      : PropSpec{std::move(name), std::move(description)},
        default_value_{default_value} {}

  /// Construct a boolean property specification from JSON.
  explicit BoolPropSpec(const nlohmann::json& json);

  /// Convert the property specification to JSON.
  auto to_json() const -> nlohmann::json override;

  /// Get the type of the property.
  constexpr auto type() const noexcept -> PropType override {
    return PropType::bool_;
  }

  /// Get the default value of the property.
  constexpr auto default_value() const noexcept -> std::optional<bool> {
    return default_value_;
  }

private:

  std::optional<bool> default_value_;

}; // class BoolPropSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Numeric property specification.
template<class Num>
class NumericPropSpec final : public PropSpec {
public:

  /// Construct a numeric property specification.
  constexpr NumericPropSpec(std::string name,
                            std::string description,
                            std::optional<Num> default_value = {},
                            std::optional<Num> min_value = {},
                            std::optional<Num> max_value = {})
      : PropSpec{std::move(name), std::move(description)},
        default_value_{std::move(default_value)},
        min_value_{std::move(min_value)}, max_value_{std::move(max_value)} {
    validate_default_and_range_();
  }

  /// Construct a numeric property specification from JSON.
  explicit NumericPropSpec(const nlohmann::json& json);

  /// Convert the property specification to JSON.
  auto to_json() const -> nlohmann::json override;

  /// Get the type of the property.
  constexpr auto type() const noexcept -> PropType override {
    if constexpr (std::integral<Num>) return PropType::int_;
    else if constexpr (std::floating_point<Num>) return PropType::float_;
    else static_assert(false);
  }

  /// Get the default value of the property.
  constexpr auto default_value() const noexcept -> std::optional<Num> {
    return default_value_;
  }

  /// Get the minimum value of the property.
  constexpr auto min_value() const noexcept -> std::optional<Num> {
    return min_value_;
  }

  /// Get the maximum value of the property.
  constexpr auto max_value() const noexcept -> std::optional<Num> {
    return max_value_;
  }

private:

  void validate_default_and_range_() const;

  std::optional<Num> default_value_;
  std::optional<Num> min_value_;
  std::optional<Num> max_value_;

}; // class NumericPropSpec

/// Integer property specification.
using IntPropSpec = NumericPropSpec<int64_t>;

/// Floating-point property specification.
using FloatPropSpec = NumericPropSpec<float64_t>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// String property specification.
class StringPropSpec final : public PropSpec {
public:

  /// Construct a string property specification.
  constexpr StringPropSpec(std::string name,
                           std::string description,
                           std::optional<std::string> default_value = {})
      : PropSpec{std::move(name), std::move(description)},
        default_value_{std::move(default_value)} {}

  /// Construct a string property specification from JSON.
  explicit StringPropSpec(const nlohmann::json& json);

  /// Convert the property specification to JSON.
  auto to_json() const -> nlohmann::json override;

  /// Get the type of the property.
  constexpr auto type() const noexcept -> PropType override {
    return PropType::string;
  }

  /// Get the default value of the property.
  constexpr auto default_value() const noexcept
      -> std::optional<std::string_view> {
    return default_value_;
  }

private:

  std::optional<std::string> default_value_;

}; // class StringPropSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Record property specification.
class RecordPropSpec final : public PropSpec {
public:

  /// Construct a record property specification.
  constexpr RecordPropSpec(std::string name,
                           std::string description,
                           std::vector<PropSpecPtr> fields)
      : PropSpec{std::move(name), std::move(description)},
        fields_{std::move(fields)} {}

  /// Construct a string property specification from JSON.
  explicit RecordPropSpec(const nlohmann::json& json);

  /// Convert the property specification to JSON.
  auto to_json() const -> nlohmann::json override;

  /// Get the type of the property.
  constexpr auto type() const noexcept -> PropType override {
    return PropType::record;
  }

  /// Get the fields of the record property.
  constexpr auto fields() const noexcept -> std::span<const PropSpecPtr> {
    return fields_;
  }

private:

  std::vector<PropSpecPtr> fields_;

}; // class RecordPropSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Variant property specification.
class VariantPropSpec final : public PropSpec {
public:

  /// Construct a variant property specification.
  constexpr VariantPropSpec(std::string name,
                            std::string description,
                            std::vector<PropSpecPtr> options,
                            std::optional<std::string> default_option = {})
      : PropSpec{std::move(name), std::move(description)},
        options_{std::move(options)},
        default_option_{std::move(default_option)} {
    validate_default_option_();
  }

  /// Construct a string property specification from JSON.
  explicit VariantPropSpec(const nlohmann::json& json);

  /// Convert the property specification to JSON.
  auto to_json() const -> nlohmann::json override;

  /// Get the type of the property.
  constexpr auto type() const noexcept -> PropType override {
    return PropType::variant;
  }

  /// Get the options of the variant property.
  constexpr auto options() const noexcept -> std::span<const PropSpecPtr> {
    return std::span{options_};
  }

  /// Get the default option of the variant property.
  constexpr auto default_option() const noexcept
      -> std::optional<std::string_view> {
    return default_option_;
  }

private:

  void validate_default_option_() const;

  std::vector<PropSpecPtr> options_;
  std::optional<std::string> default_option_;

}; // class VariantPropSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
