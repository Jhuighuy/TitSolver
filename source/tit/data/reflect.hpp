/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/data/spec.hpp"
#include "tit/data/storage.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Functional object for reading parameter values from a data series.
class Reader final {
public:

  /// Construct a reader to the given data series.
  explicit Reader(DataSeriesView<const DataStorage> series) : series_{series} {}

  /// Read a boolean parameter value.
  void operator()(bool& val, const json::json& spec) const {
    TIT_ASSERT(spec["type"] == "bool", "Expected boolean parameter!");
    val = find_param_(spec).value() == "true";
  }

  /// Read an integer parameter value.
  template<std::integral Int>
  void operator()(Int& val, const json::json& spec) const {
    TIT_ASSERT(spec["type"] == "int", "Expected integer parameter!");
    const auto parsed = str_to<Int>(find_param_(spec).value());
    TIT_ENSURE(parsed.has_value(), "Invalid integer parameter value!");
    val = parsed.value();
  }

  /// Read a floating-point parameter value.
  template<std::floating_point Float>
  void operator()(Float& val, const json::json& spec) const {
    TIT_ASSERT(spec["type"] == "float", "Expected floating-point parameter!");
    const auto parsed = str_to<Float>(find_param_(spec).value());
    TIT_ENSURE(parsed.has_value(), "Invalid floating-point parameter value!");
    val = parsed.value();
  }

  /// Read a string parameter value.
  void operator()(std::string& val, const json::json& spec) const {
    TIT_ASSERT(spec["type"] == "string", "Expected string parameter!");
    val = find_param_(spec).value();
  }

  /// Read a record parameter value.
  template<class Val>
  void operator()(Val& val, const json::json& spec) const {
    TIT_ASSERT(spec["type"] == "record", "Expected record parameter!");
    const auto param = find_param_(spec);
    parent_ids_.push_back(param.id());
    val.reflect(*this);
    parent_ids_.pop_back();
  }

private:

  /// @todo This is a very naive and inefficient implementation.
  auto find_param_(const json::json& spec) const
      -> DataParamView<const DataStorage> {
    const auto parent_id = parent_ids_.back();
    for (const auto& param : series_.params()) {
      if (param.parent_id() != parent_id) continue;
      if (param.spec()->to_json()["name"] == spec["name"]) return param;
    }
    TIT_THROW("Parameter not found!");
  }

  DataSeriesView<const DataStorage> series_;
  mutable std::vector<DataParamID> parent_ids_{DataParamID{0}};

}; // class Reader

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Functional object for writing parameter values to a data series.
class Writer final {
public:

  /// Construct a writer to the given data series.
  explicit Writer(DataSeriesView<DataStorage> series) : series_{series} {}

  /// Write a boolean parameter value.
  void operator()(const bool& val, json::json spec) const {
    spec["type"] = "bool";
    series_.create_param(*Spec::from_json(std::move(spec)),
                         parent_ids_.back(),
                         val ? "true" : "false");
  }

  /// Write an integer parameter value.
  template<std::integral Int>
  void operator()(const Int& val, json::json spec) const {
    spec["type"] = "int";
    series_.create_param(*Spec::from_json(std::move(spec)),
                         parent_ids_.back(),
                         std::to_string(val));
  }

  /// Write a floating-point parameter value.
  template<std::floating_point Float>
  void operator()(const Float& val, json::json spec) const {
    spec["type"] = "float";
    series_.create_param(*Spec::from_json(std::move(spec)),
                         parent_ids_.back(),
                         std::to_string(val));
  }

  /// Write a string parameter value.
  void operator()(const std::string& val, json::json spec) const {
    spec["type"] = "string";
    series_.create_param(*Spec::from_json(std::move(spec)),
                         parent_ids_.back(),
                         val);
  }

  /// Write a record parameter value.
  template<class Val>
  void operator()(const Val& val, json::json spec) const {
    spec["type"] = "record";
    const auto param = series_.create_param(*Spec::from_json(std::move(spec)),
                                            parent_ids_.back());

    parent_ids_.push_back(param.id());
    val.reflect(*this);
    parent_ids_.pop_back();
  }

private:

  DataSeriesView<DataStorage> series_;
  mutable std::vector<DataParamID> parent_ids_{DataParamID{0}};

}; // class Writer

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
