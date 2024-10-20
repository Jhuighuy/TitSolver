/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <map>
#include <string>

#include "tit/core/checks.hpp"

#include "tit/data/darray.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Dataset.
class Dataset final {
public:

  /// Does the dataset contain a constant with the given name?
  constexpr auto has_constant(const std::string& name) const noexcept {
    return constants_.contains(name);
  }

  /// Does the dataset contain a variable with the given name?
  constexpr auto has_variable(const std::string& name) const noexcept {
    return variables_.contains(name);
  }

  /// Get a constant with the given name.
  /// @{
  constexpr auto constant(const std::string& name) -> DataArray& {
    TIT_ASSERT(has_constant(name),
               "Constant with the given name does not exist!");
    return constants_.at(name);
  }
  constexpr auto constant(const std::string& name) const -> const DataArray& {
    TIT_ASSERT(has_constant(name),
               "Constant with the given name does not exist!");
    return constants_.at(name);
  }
  /// @}

  /// Get a variable with the given name.
  /// @{
  constexpr auto variable(const std::string& name) -> DataArray& {
    TIT_ASSERT(has_variable(name),
               "Variable with the given name does not exist!");
    return variables_.at(name);
  }
  constexpr auto variable(const std::string& name) const -> const DataArray& {
    TIT_ASSERT(has_variable(name),
               "Variable with the given name does not exist!");
    return variables_.at(name);
  }
  /// @}

private:

  std::map<std::string, DataArray> constants_;
  std::map<std::string, DataArray> variables_;

}; // class Dataset

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
