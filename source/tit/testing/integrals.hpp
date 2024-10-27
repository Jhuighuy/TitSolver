/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <functional>
#include <numbers>
#include <ranges>
#include <type_traits>

#include "tit/core/basic_types.hpp"
#include "tit/core/math.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Integral over a single segment.
template<class Func, class Num>
constexpr auto integrate_piece(Func&& f, const geom::BBox<Vec<Num, 1>>& box) {
  TIT_ASSUME_UNIVERSAL(Func, f);
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
template<class Func, class Num, size_t Dim>
constexpr auto integrate_piece(Func&& f, const geom::BBox<Vec<Num, Dim>>& box) {
  TIT_ASSUME_UNIVERSAL(Func, f);
  const geom::BBox box_head{vec_head(box.low()), vec_head(box.high())};
  const geom::BBox box_tail{vec_tail(box.low()), vec_tail(box.high())};
  return integrate_piece(
      [&f, &box_head](const Vec<Num, Dim - 1>& y) {
        return integrate_piece(
            [&f, &y](const Vec<Num, 1>& x) { return f(vec_cat(x, y)); },
            box_head);
      },
      box_tail);
}

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Integrate a function over a box.
template<class Func, class Num, size_t Dim>
constexpr auto integrate(Func&& f,
                         const geom::BBox<Vec<Num, Dim>>& box,
                         std::type_identity_t<Num> eps = tiny_number_v<Num>) {
  TIT_ASSUME_UNIVERSAL(Func, f);
  return [&f](this auto self,
              const geom::BBox<Vec<Num, Dim>>& my_box,
              const auto& estimate,
              Num tolerance) {
    // Split the box into pieces.
    const auto center = my_box.center();
    const auto pieces = my_box.split(center);

    // Integrate the pieces.
    const auto piece_integrals = std::apply(
        [&f](const auto&... ps) {
          return std::array{impl::integrate_piece(f, ps)...};
        },
        pieces);

    // Calculate the total integral value.
    auto integral = std::apply([](const auto&... is) { return (is + ...); },
                               piece_integrals);
    if (abs(integral - estimate) <= tolerance) return integral;

    // Recurse into the pieces.
    integral = {};
    for (const auto& [piece, piece_integral] :
         std::views::zip(pieces, piece_integrals)) {
      integral += self(piece, piece_integral, tolerance / 2);
    }
    return integral;
  }(box, impl::integrate_piece(f, box), eps);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Integrate a function over a circle.
template<class Func, class Num>
constexpr auto integrate_cr(
    Func&& f,
    const Num& radius,
    std::type_identity_t<Num> eps = tiny_number_v<Num>) {
  TIT_ASSUME_UNIVERSAL(Func, f);
  const auto from_polar = [&f](const Vec<Num, 2>& point) {
    const auto& r = point[0];
    const auto& phi = point[1];
    const auto x = r * cos(phi);
    const auto y = r * sin(phi);
    const auto J = r;
    return J * std::invoke(f, Vec{x, y});
  };
  const geom::BBox box{Vec<Num, 2>{}, Vec{radius, Num{2 * std::numbers::pi}}};
  return integrate(from_polar, box, eps);
}

/// Integrate a function over a sphere.
template<class Func, class Num>
constexpr auto integrate_sp(
    Func&& f,
    const Num& radius,
    std::type_identity_t<Num> eps = tiny_number_v<Num>) {
  TIT_ASSUME_UNIVERSAL(Func, f);
  const auto from_spherical = [&f](const Vec<Num, 3>& point) {
    const auto& r = point[0];
    const auto& theta = point[1];
    const auto& phi = point[2];
    const auto x = r * sin(theta) * cos(phi);
    const auto y = r * sin(theta) * sin(phi);
    const auto z = r * cos(theta);
    const auto J = pow2(r) * sin(theta);
    return J * std::invoke(f, Vec{x, y, z});
  };
  using std::numbers::pi;
  const geom::BBox box{Vec<Num, 3>{}, Vec{radius, Num{pi}, Num{2.0 * pi}}};
  return integrate(from_spherical, box, eps);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
