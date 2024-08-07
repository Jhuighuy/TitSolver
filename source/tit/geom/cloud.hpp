/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <ranges>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Cloud of points.
template<class Points>
concept cloud = std::ranges::sized_range<Points> &&
                std::ranges::random_access_range<Points> &&
                is_vec_v<std::ranges::range_value_t<Points>>;

/// A view on a cloud of points.
template<class Points>
concept cloud_view =
    std::ranges::view<Points> && cloud<std::views::all_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// A view on a permuted cloud of points.
template<std::ranges::viewable_range Points, std::ranges::viewable_range Perm>
  requires cloud<Points> &&
               (std::ranges::sized_range<Perm> &&
                std::ranges::random_access_range<Perm> &&
                std::unsigned_integral<std::ranges::range_value_t<Perm>>)
constexpr auto permuted_cloud(Points&& points,
                              Perm&& perm) noexcept -> cloud_view auto {
  return std::views::all(std::forward<Perm>(perm)) |
         std::views::transform(
             [points_view = std::views::all(std::forward<Points>(points))](
                 size_t i) -> decltype(auto) {
               TIT_ASSERT(i < points_view.size(), "Index is out of range!");
               return points_view[i];
             });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Calculate the cloud bounding box.
template<cloud Points>
constexpr auto cloud_bbox(Points&& points) {
  TIT_ASSUME_UNIVERSAL(Points, points);
  BBox box{points[0]};
  for (const auto& p : points | std::views::drop(1)) box.expand(p);
  return box;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Calculate the simplified cloud inertia tensor.
///
/// The true inertia tensor is ∑(rᵢ·rᵢI - rᵢ⊗rᵢ), where rᵢ is the position
/// vector of the i-th point relative to the center of mass. In most of the
/// applications just the eigenvectors of the inertia tensor are used for the
/// analysis. Since the first term is a scalar multiple of the identity matrix,
/// it does not affect the eigenvectors of the inertia tensor. Thus, we can
/// simplify the computation to ∑(rᵢ⊗rᵢ).
template<cloud Points>
constexpr auto cloud_inertia_tensor(Points&& points) {
  TIT_ASSUME_UNIVERSAL(Points, points);
  auto sum = points[0];
  auto inertia_tensor = outer_sqr(points[0]);
  for (const auto& p : points | std::views::drop(1)) {
    sum += p;
    inertia_tensor += outer_sqr(p);
  }
  inertia_tensor -= outer(sum, sum / points.size());
  return inertia_tensor;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
