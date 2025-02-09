/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Is the type a flag enum?
template<class Enum>
  requires (std::is_enum_v<Enum> &&
            std::unsigned_integral<std::underlying_type_t<Enum>>)
constexpr bool is_flags_enum_v = false;

/// Is the type a flags enum?
template<class Enum>
concept flags_enum = std::is_enum_v<Enum> &&
                     std::unsigned_integral<std::underlying_type_t<Enum>> &&
                     is_flags_enum_v<Enum>;

/// Merge two flags enums.
template<flags_enum Enum>
constexpr auto operator|(Enum f, Enum g) noexcept -> Enum {
  return static_cast<Enum>(std::to_underlying(f) | std::to_underlying(g));
}

/// Intersect two flags enum.
template<flags_enum Enum>
constexpr auto operator&(Enum f, Enum g) noexcept -> bool {
  return (std::to_underlying(f) & std::to_underlying(g)) != 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
