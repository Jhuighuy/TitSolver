/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <numbers>
#include <ranges>
#include <type_traits>

#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"
#include "tit/geom/bsphere.hpp"
#include "tit/geom/segment.hpp"
#include "tit/geom/triangle.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Integrate a function using a fixed Kronrod 15 quadrature rule.
template<class Num>
constexpr auto integrate_piece(auto&& f, const geom::BBox<Vec<Num, 1>>& box) {
  static constexpr auto x_gk = std::to_array<Num>({
      0.000000000000000000000000000000000,
      0.207784955007898467600689403773245,
      0.405845151377397166906606412076961,
      0.586087235467691130294144838258730,
      0.741531185599394439863864773280788,
      0.864864423359769072789712788640926,
      0.949107912342758524526189684047851,
      0.991455371120812639206854697526329,
  });

  static constexpr auto w_gk = std::to_array<Num>({
      0.209482141084727828012999174891714,
      0.204432940075298892414161999234649,
      0.190350578064785409913256402421014,
      0.169004726639267902826583426598550,
      0.140653259715525918745189590510238,
      0.104790010322250183839876322541518,
      0.063092092629978553290700663189204,
      0.022935322010529224963732008058970,
  });

  const auto& a = box.low()[0];
  const auto& b = box.high()[0];
  const auto center = avg(a, b);
  const auto half_h = center - a;

  auto sum = w_gk.front() * std::invoke(f, Vec{center});
  for (const auto [x, w] : std::views::zip(x_gk, w_gk) | std::views::drop(1)) {
    sum += w * (std::invoke(f, Vec{center + x * half_h}) +
                std::invoke(f, Vec{center - x * half_h}));
  }

  return sum * half_h;
}

// Integrate a function using a fixed Genz-Malik 7 quadrature rule.
template<class Num, std::size_t Dim>
constexpr auto integrate_piece(auto&& f, const geom::BBox<Vec<Num, Dim>>& box) {
  constexpr auto lambda2 = sqrt(Num{9} / Num{70});
  constexpr auto lambda3 = sqrt(Num{9} / Num{10});
  constexpr auto lambda5 = sqrt(Num{9} / Num{19});

  constexpr auto w1 = pow(2, Dim) *
                      (Num{12824} - (Num{9120} - Num{400} * Dim) * Dim) /
                      Num{19683};
  constexpr auto w2 = pow(2, Dim) * Num{980} / Num{6561};
  constexpr auto w3 = pow(2, Dim) * (Num{1820} - Num{400} * Dim) / Num{19683};
  constexpr auto w4 = pow(2, Dim) * Num{200} / Num{19683};
  constexpr auto w5 = Num{6859} / Num{19683};

  const auto center = box.center();
  const auto half_extents = box.extents() / Num{2};

  const auto p1 = center;
  auto sum = w1 * std::invoke(f, p1);

  for (std::size_t i = 0; i < Dim; ++i) {
    for (const auto si : {-1, 1}) {
      auto p2 = center;
      p2[i] += si * lambda2 * half_extents[i];
      sum += w2 * std::invoke(f, p2);

      auto p3 = center;
      p3[i] += si * lambda3 * half_extents[i];
      sum += w3 * std::invoke(f, p3);
    }
  }

  for (std::size_t i = 0; i + 1 < Dim; ++i) {
    for (std::size_t j = i + 1; j < Dim; ++j) {
      for (const auto si : {-1, 1}) {
        for (const auto sj : {-1, 1}) {
          auto p4 = center;
          p4[i] += si * lambda3 * half_extents[i];
          p4[j] += sj * lambda3 * half_extents[j];
          sum += w4 * std::invoke(f, p4);
        }
      }
    }
  }

  for (std::size_t mask = 0; mask < pow(std::size_t{2}, Dim); ++mask) {
    auto p5 = center;
    for (std::size_t k = 0; k < Dim; ++k) {
      const auto sk = ((mask >> k) & 1U) != 0 ? 1 : -1;
      p5[k] += sk * lambda5 * half_extents[k];
    }
    sum += w5 * std::invoke(f, p5);
  }

  return sum * prod(half_extents);
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
        approx_equal_to(total_integral, estimate, tolerance)) {
      return total_integral;
    }
    return (self(pieces, piece_integrals, tolerance / pow(2, Dim)) + ...);
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
    return integrate(f, geom::BBox{domain.center()}.grow(domain.radius()), eps);
  } else if constexpr (Dim == 2) {
    const auto from_polar = [&f, &domain](const Vec<Num, 2>& point) {
      const auto& [r, phi] = point.elems();
      const auto x = r * cos(phi);
      const auto y = r * sin(phi);
      const auto J = r;
      return J * std::invoke(f, Vec{x, y} + domain.center());
    };
    return integrate(
        from_polar,
        geom::BBox<Vec<Num, 2>>{{}, {domain.radius(), Num{2.0 * pi}}},
        eps);
  } else if constexpr (Dim == 3) {
    const auto from_spherical = [&f, &domain](const Vec<Num, 3>& point) {
      const auto& [r, theta, phi] = point.elems();
      const auto x = r * sin(theta) * cos(phi);
      const auto y = r * sin(theta) * sin(phi);
      const auto z = r * cos(theta);
      const auto J = pow2(r) * sin(theta);
      return J * std::invoke(f, Vec{x, y, z} + domain.center());
    };
    return integrate(
        from_spherical,
        geom::BBox<Vec<Num, 3>>{{}, {domain.radius(), Num{pi}, Num{2.0 * pi}}},
        eps);
  } else {
    static_assert(false);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Integrate a function over a segment.
/// @todo Tests are missing.
template<class Num, std::size_t Dim>
constexpr auto integrate(auto&& f,
                         const geom::Segment<Vec<Num, Dim>>& seg,
                         std::type_identity_t<Num> eps = tiny_v<Num>) {
  const auto J = seg.length();
  const auto from_segment = [&J, &f, &seg](const Vec<Num, 1>& t) {
    const auto y = seg.a() + t[0] * seg.ba();
    return J * std::invoke(f, y);
  };
  return integrate(from_segment, geom::BBox<Vec<Num, 1>>{{0}, {1}}, eps);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Integrate a function over a triangle.
/// @todo Tests are missing.
template<class Num, std::size_t Dim>
constexpr auto integrate(auto&& f,
                         const geom::Triangle<Vec<Num, Dim>>& tri,
                         std::type_identity_t<Num> eps = tiny_v<Num>) {
  const auto J = tri.area();
  const auto from_triangle = [&J, &f, &tri](const Vec<Num, 2>& t) {
    const auto y =
        tri.a() + t[0] * ((Num{1} - t[1]) * tri.ba() + t[1] * tri.ca());
    return Num{2} * t[0] * J * std::invoke(f, y);
  };
  return integrate(from_triangle, geom::BBox<Vec<Num, 2>>{{0, 0}, {1, 1}}, eps);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
