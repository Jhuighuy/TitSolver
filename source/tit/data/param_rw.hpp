/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <string>
#include <utility>

#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/core/utils.hpp"
#include "tit/data/json.hpp"
#include "tit/data/param_spec.hpp"
#include "tit/data/storage.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Functional object for reading parameter values from a data series.
class ParamReader final {
public:

  /// Construct a reader to the given data series.
  explicit ParamReader(DataSeriesView<const DataStorage> series)
      : series_{series} {}

  /// Read a boolean parameter value.
  void operator()(bool& val, const json::json& spec) const {
    TIT_ASSERT(param_spec_type_from_json(spec["type"]) == ParamSpecType::bool_,
               "Expected boolean parameter!");
    const auto val_str = find_param_(spec).value();
    TIT_ALWAYS_ASSERT(val_str == "true" || val_str == "false",
                      "Invalid boolean parameter value!");
    val = val_str == "true";
  }

  /// Read an integer parameter value.
  template<std::integral Int>
  void operator()(Int& val, const json::json& spec) const {
    TIT_ASSERT(param_spec_type_from_json(spec["type"]) == ParamSpecType::int_,
               "Expected integer parameter!");
    const auto parsed = str_to<Int>(find_param_(spec).value());
    TIT_ALWAYS_ASSERT(parsed.has_value(), "Invalid integer parameter value!");
    val = parsed.value();
  }

  /// Read a floating-point parameter value.
  template<std::floating_point Float>
  void operator()(Float& val, const json::json& spec) const {
    TIT_ASSERT(param_spec_type_from_json(spec["type"]) == ParamSpecType::float_,
               "Expected floating-point parameter!");
    const auto parsed = str_to<Float>(find_param_(spec).value());
    TIT_ALWAYS_ASSERT(parsed.has_value(),
                      "Invalid floating-point parameter value!");
    val = parsed.value();
  }

  /// Read a string parameter value.
  void operator()(std::string& val, const json::json& spec) const {
    TIT_ASSERT(param_spec_type_from_json(spec["type"]) == ParamSpecType::str,
               "Expected string parameter!");
    val = find_param_(spec).value();
  }

  /// Read a record parameter value.
  template<class Val>
  void operator()(Val& val, const json::json& spec) const {
    TIT_ASSERT(param_spec_type_from_json(spec["type"]) == ParamSpecType::record,
               "Expected record parameter!");
    const auto param = find_param_(spec);
    const ScopedVal parent_id{parent_id_, param.id()};
    val.reflect(*this);
  }

private:

  /// @todo This is a very naive and inefficient implementation.
  auto find_param_(const json::json& spec) const
      -> DataParamView<const DataStorage> {
    for (const auto& param : series_.params()) {
      if (param.parent_id() != parent_id_) continue;
      if (param.spec()->to_json()["name"] == spec["name"]) return param;
    }
    TIT_THROW("Parameter not found!");
  }

  DataSeriesView<const DataStorage> series_;
  mutable DataParamID parent_id_{0};

}; // class Reader

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Functional object for writing parameter values to a data series.
class ParamWriter final {
public:

  /// Construct a writer to the given data series.
  explicit ParamWriter(DataSeriesView<DataStorage> series) : series_{series} {}

  /// Write a boolean parameter value.
  void operator()(const bool& val, json::json spec) const {
    spec["type"] = param_spec_type_to_string(ParamSpecType::bool_);
    series_.create_param(*ParamSpec::from_json(std::move(spec)),
                         parent_id_,
                         val ? "true" : "false");
  }

  /// Write an integer parameter value.
  template<std::integral Int>
  void operator()(const Int& val, json::json spec) const {
    spec["type"] = param_spec_type_to_string(ParamSpecType::int_);
    series_.create_param(*ParamSpec::from_json(std::move(spec)),
                         parent_id_,
                         std::to_string(val));
  }

  /// Write a floating-point parameter value.
  template<std::floating_point Float>
  void operator()(const Float& val, json::json spec) const {
    spec["type"] = param_spec_type_to_string(ParamSpecType::float_);
    series_.create_param(*ParamSpec::from_json(std::move(spec)),
                         parent_id_,
                         std::to_string(val));
  }

  /// Write a string parameter value.
  void operator()(const std::string& val, json::json spec) const {
    spec["type"] = param_spec_type_to_string(ParamSpecType::str);
    series_.create_param(*ParamSpec::from_json(std::move(spec)),
                         parent_id_,
                         val);
  }

  /// Write a record parameter value.
  template<class Val>
  void operator()(const Val& val, json::json spec) const {
    spec["type"] = param_spec_type_to_string(ParamSpecType::record);
    const auto param =
        series_.create_param(*ParamSpec::from_json(std::move(spec)),
                             parent_id_);
    const ScopedVal parent_id{parent_id_, param.id()};
    val.reflect(*this);
  }

private:

  DataSeriesView<DataStorage> series_;
  mutable DataParamID parent_id_{0};

}; // class Writer

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
