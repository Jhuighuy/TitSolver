/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <tuple> // IWYU pragma: keep
#include <utility>

namespace tit {

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
