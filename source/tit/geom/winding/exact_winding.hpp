/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/vec.hpp"
#include "tit/geom/surface.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Generalized winding function for a surface.
template<class Vec>
  requires is_vec_v<Vec> && (vec_dim_v<Vec> == 2 || vec_dim_v<Vec> == 3)
class ExactWindingFunc final {
public:

  /// Numeric type.
  using Num = vec_num_t<Vec>;

  /// Temporary surfaces are not permitted.
  constexpr explicit ExactWindingFunc(Surface<Vec>&&) = delete;

  /// Construct an exact winding number function for the surface.
  constexpr explicit ExactWindingFunc(const Surface<Vec>& surf)
      : surf_{&surf} {}

  /// Estimate the generalized winding number at the point.
  constexpr auto operator()(const Vec& point) const noexcept -> Num {
    Num result{};
    for (const auto& face : surf_->faces()) {
      result += face.winding_number(point);
    }
    return result;
  }

  /// Test whether the winding number is above a threshold.
  constexpr auto contains(const Vec& point,
                          Num threshold = Num{0.5}) const noexcept -> bool {
    return (*this)(point) > threshold;
  }

private:

  const Surface<Vec>* surf_;

}; // class ExactWindingFunc

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Exact winding number function constructor.
class MakeExactWinding final {
public:

  /// Construct an exact winding number function for the surface.
  template<class Vec>
    requires is_vec_v<Vec>
  [[nodiscard]] static auto operator()(const Surface<Vec>& surf) {
    return ExactWindingFunc{surf};
  }

  /// Temporary surfaces are not permitted.
  template<class Vec>
  constexpr static auto operator()(Surface<Vec>&&) = delete;

}; // class MakeExactWinding

/// Construct an exact winding number function for the surface.
inline constexpr MakeExactWinding make_exact_winding{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
