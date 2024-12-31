/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <string_view>

namespace tit::py {

class Object;
class Bool;
class Int;
class Float;
class Str;

template<class T>
concept str_like = std::constructible_from<std::string_view, T>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct keep_t {};

/// @copydoc keep_t
inline constexpr keep_t keep{};

struct copy_t {};

/// @copydoc copy_t
inline constexpr copy_t copy{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Convert the argument to the given type.
template<class To = class Object, class From>
  requires std::derived_from<To, Object> || std::derived_from<From, Object>
auto cast(const From& arg) -> To {
  static constexpr bool FromPython = std::derived_from<From, Object>;
  static constexpr bool ToPython = std::derived_from<To, Object>;
  if constexpr (FromPython && ToPython) {
    // Cast the argument to the matching Python type, and keep it.
    if constexpr (std::derived_from<From, To>) return arg;
    else return To{arg, keep};
  } else if constexpr (FromPython) {
    // Cast the argument to the matching Python type, and get the C++ value.
    if constexpr (std::same_as<To, bool>) return cast<Bool>(arg).val();
    else if constexpr (std::integral<To>) return cast<Int>(arg).val();
    else if constexpr (std::floating_point<To>) return cast<Float>(arg).val();
    else if constexpr (str_like<To>) return cast<Str>(arg).val();
    else static_assert(false, "Unsupported Python to C++ cast!");
  } else if constexpr (ToPython) {
    // Construct a new object from the argument, and cast it to the target type.
    if constexpr (std::same_as<From, bool>) return cast<To>(Bool{arg});
    else if constexpr (std::integral<From>) return cast<To>(Int{arg});
    else if constexpr (std::floating_point<From>) return cast<To>(Float{arg});
    else if constexpr (str_like<From>) return cast<To>(Str{arg});
    else static_assert(false, "Unsupported C++ to Python cast!");
  } else static_assert(false, "C++ to C++ casts are not supported!");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
