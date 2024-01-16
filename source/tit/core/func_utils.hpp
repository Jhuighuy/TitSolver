/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <tuple> // IWYU pragma: keep
#include <utility>

#include "tit/core/types.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Functor that gets tuple element at index. */
template<size_t Index>
struct GetFn {
  template<class Tuple>
  constexpr auto operator()(Tuple&& tuple) const noexcept -> decltype(auto) {
    return std::get<Index>(std::forward<Tuple>(tuple));
  }
}; // struct GetFn

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// TODO: this definitely needs some rework.
template<class Func>
class OnAssignment {
private:

  Func func_;

public:

  constexpr explicit OnAssignment(Func func) : func_{std::move(func)} {}

  template<class Arg>
  // NOLINTNEXTLINE(*-copy-assignment-signature,*-unconventional-assign-operator)
  constexpr void operator=(Arg&& arg) {
    func_(std::forward<Arg>(arg));
  }

}; // class OnAssignment

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
