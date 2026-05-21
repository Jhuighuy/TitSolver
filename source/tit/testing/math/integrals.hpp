/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <numbers>
#include <type_traits>

#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"
#include "tit/geom/bsphere.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class Num>
constexpr auto integrate_piece(auto&& f, const geom::BBox<Vec<Num, 1>>& box) {
  const auto& a = box.low()[0];
  const auto& b = box.high()[0];
  const auto c = avg(a, b);
  const auto half_h = c - a;
  const auto w = half_h * static_cast<Num>(sqrt(0.6));
  return (Num{8.0 / 9.0} * std::invoke(f, Vec{c}) +
          Num{5.0 / 9.0} * (std::invoke(f, Vec{c + w}) + //
                            std::invoke(f, Vec{c - w}))) *
         half_h;
}

template<class Num, std::size_t Dim>
constexpr auto integrate_piece(auto&& f, const geom::BBox<Vec<Num, Dim>>& box) {
  const geom::BBox box_head{vec_head(box.low()), vec_head(box.high())};
  const geom::BBox box_tail{vec_tail(box.low()), vec_tail(box.high())};
  return integrate_piece(
      [&f, &box_head](const Vec<Num, Dim - 1>& y) {
        return integrate_piece(
            [&f, &y](const Vec<Num, 1>& x) {
              return std::invoke(f, vec_cat(x, y));
            },
            box_head);
      },
      box_tail);
}

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Integrate a function over a box.
template<class Num, std::size_t Dim>
constexpr auto integrate(auto&& f,
                         const geom::BBox<Vec<Num, Dim>>& box,
                         std::type_identity_t<Num> eps = tiny_v<Num>) {
  return [&f](this auto self,
              const geom::BBox<Vec<Num, Dim>>& my_box,
              const auto& estimate,
              Num tolerance) {
    const auto [... pieces] = my_box.split(my_box.center());
    const auto [... piece_integrals] =
        std::array{impl::integrate_piece(f, pieces)...};
    if (const auto total_integral = (piece_integrals + ...);
        abs(total_integral - estimate) <= tolerance) {
      return total_integral;
    }
    return (self(pieces, piece_integrals, tolerance / 2) + ...);
  }(box, impl::integrate_piece(f, box), eps);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Integrate a function over a sphere.
template<class Num, std::size_t Dim>
constexpr auto integrate(auto&& f,
                         const geom::BSphere<Vec<Num, Dim>>& domain,
                         std::type_identity_t<Num> eps = tiny_v<Num>) {
  using std::numbers::pi;
  if constexpr (Dim == 1) {
    const geom::BBox box{
        domain.center() - Vec{domain.radius()},
        domain.center() + Vec{domain.radius()},
    };
    return integrate(f, box, eps);
  } else if constexpr (Dim == 2) {
    const auto from_polar = [&f, &domain](const Vec<Num, 2>& point) {
      const auto& [r, phi] = point.elems();
      const auto x = r * cos(phi);
      const auto y = r * sin(phi);
      const auto J = r;
      return J * std::invoke(f, Vec{x, y} + domain.center());
    };
    const geom::BBox box{
        Vec<Num, 2>{},
        Vec{domain.radius(), Num{2.0 * pi}},
    };
    return integrate(from_polar, box, eps);
  } else if constexpr (Dim == 3) {
    const auto from_spherical = [&f, &domain](const Vec<Num, 3>& point) {
      const auto& [r, theta, phi] = point.elems();
      const auto x = r * sin(theta) * cos(phi);
      const auto y = r * sin(theta) * sin(phi);
      const auto z = r * cos(theta);
      const auto J = pow2(r) * sin(theta);
      return J * std::invoke(f, Vec{x, y, z} + domain.center());
    };
    const geom::BBox box{
        Vec<Num, 3>{},
        Vec{domain.radius(), Num{pi}, Num{2.0 * pi}},
    };
    return integrate(from_spherical, box, eps);
  } else {
    static_assert(false);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
