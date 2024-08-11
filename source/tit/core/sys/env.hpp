/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/sys.hpp"
#pragma once

#include <optional>
#include <string_view>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Get the value of an environment variable.
auto get_env(const char* name) noexcept -> std::optional<std::string_view>;

/// Get the value of an environment variable as an integer.
/// @{
auto get_env_int(const char* name) noexcept -> std::optional<int64_t>;
auto get_env_int(const char* name, int64_t fallback) noexcept -> int64_t;
/// @}

/// Get the value of an environment variable as a floating-point.
/// @{
auto get_env_float(const char* name) noexcept -> std::optional<float64_t>;
auto get_env_float(const char* name, float64_t fallback) noexcept -> float64_t;
/// @}

/// Get the value of an environment variable as a boolean.
/// @{
auto get_env_bool(const char* name) noexcept -> std::optional<bool>;
auto get_env_bool(const char* name, bool fallback) noexcept -> bool;
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
