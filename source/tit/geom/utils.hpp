/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <expected>
#include <iterator>
#include <ranges>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Range of points.
template<class Points>
concept point_range = std::ranges::sized_range<Points> &&
                      std::ranges::random_access_range<Points> &&
                      is_vec_v<std::ranges::range_value_t<Points>>;

/// Point range type.
template<point_range Points>
using point_range_vec_t = std::ranges::range_value_t<Points>;

/// Point range matrix type.
template<point_range Points>
using point_range_mat_t = decltype(diag(point_range_vec_t<Points>{}));

/// Point range coordinate type.
template<point_range Points>
using point_range_num_t = vec_num_t<point_range_vec_t<Points>>;

/// Point range dimension.
template<point_range Points>
inline constexpr auto point_range_dim_v = vec_dim_v<point_range_vec_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Range of indices.
template<class Indices>
concept index_range = std::ranges::random_access_range<Indices> &&
                      std::integral<std::ranges::range_value_t<Indices>>;

/// Range of indices that can be used as output for various algorithms.
template<class Indices>
concept output_index_range = std::ranges::random_access_range<Indices> &&
                             std::ranges::output_range<Indices, size_t>;

/// Permuted range of points.
template<point_range Points, index_range Perm>
  requires std::ranges::viewable_range<Points> &&
               std::ranges::viewable_range<Perm>
constexpr auto permuted_points(Points&& points, Perm&& perm) -> //
    point_range auto {
  return std::ranges::transform_view{
      std::forward<Perm>(perm),
      [points_view = std::views::all(std::forward<Points>(points))](
          size_t index) -> const auto& { return points_view[index]; }};
}

/// Initialize the permutation with the identity.
/// @{
template<point_range Points>
[[nodiscard]] constexpr auto iota_perm(Points&& points) noexcept {
  TIT_ASSUME_UNIVERSAL(Points, points);
  return std::views::iota(size_t{0}, std::size(points));
}
template<point_range Points, output_index_range Perm>
constexpr void iota_perm(Points&& points, Perm&& perm) {
  TIT_ASSUME_UNIVERSAL(Points, points);
  TIT_ASSUME_UNIVERSAL(Perm, perm);
  std::ranges::copy(iota_perm(points), std::begin(perm));
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Copy the points into the output, filtering by the predicate.
template<point_range Points,
         index_range Perm,
         std::output_iterator<size_t> OutIter,
         std::predicate<point_range_vec_t<Points>> Pred>
constexpr auto copy_points_if(Points&& points,
                              Perm&& perm,
                              OutIter out,
                              Pred pred) -> OutIter {
  TIT_ASSUME_UNIVERSAL(Points, points);
  TIT_ASSUME_UNIVERSAL(Perm, perm);
  const auto result = std::ranges::copy_if( //
      perm,
      out,
      pred,
      [&points](size_t i) -> const auto& { return points[i]; });
  return result.out;
}

/// Copy points that are close to the given point.
template<point_range Points,
         index_range Perm,
         std::output_iterator<size_t> OutIter>
constexpr auto copy_points_near(Points&& points,
                                Perm&& perm,
                                OutIter out,
                                const point_range_vec_t<Points>& point,
                                point_range_num_t<Points> r_sqr) -> OutIter {
  TIT_ASSUME_UNIVERSAL(Points, points);
  TIT_ASSUME_UNIVERSAL(Perm, perm);
  return copy_points_if( //
      points,
      perm,
      out,
      [&point, r_sqr](const auto& p) { return norm2(p - point) < r_sqr; });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Count the number of points in the given range as a point range number type.
template<point_range Points>
constexpr auto count_points(Points&& points) -> point_range_num_t<Points> {
  TIT_ASSUME_UNIVERSAL(Points, points);
  return static_cast<point_range_num_t<Points>>(std::size(points));
}

/// Compute the centroid of the given points.
/// @{
template<point_range Points>
constexpr auto compute_center(Points&& points) -> point_range_vec_t<Points> {
  TIT_ASSUME_UNIVERSAL(Points, points);
  TIT_ASSERT(!std::ranges::empty(points), "Points must not be empty!");
  auto sum = *std::begin(points);
  for (const auto& point : points | std::views::drop(1)) sum += point;
  return sum / count_points(points);
}
template<point_range Points, index_range Perm>
constexpr auto compute_center(Points&& points,
                              Perm&& perm) -> point_range_vec_t<Points> {
  TIT_ASSUME_UNIVERSAL(Points, points);
  TIT_ASSUME_UNIVERSAL(Perm, perm);
  return compute_center(permuted_points(points, perm));
}
/// @}

/// Compute the inertia tensor of the given points.
///
/// Inertia tensor is defined as ∑rᵢ⊗rᵢ, where rᵢ is the position vector of
/// the i-th point relative to the center of mass.
/// @{
template<point_range Points>
constexpr auto compute_inertia_tensor(Points&& points)
    -> point_range_mat_t<Points> {
  TIT_ASSUME_UNIVERSAL(Points, points);
  TIT_ASSERT(!std::ranges::empty(points), "Points must not be empty!");
  auto sum = *std::begin(points);
  auto inertia_tensor = outer_sqr(sum);
  for (const auto& point : points | std::views::drop(1)) {
    sum += point;
    inertia_tensor += outer_sqr(point);
  }
  const auto center = sum / count_points(points);
  inertia_tensor -= outer(sum, center);
  return inertia_tensor;
}
template<point_range Points, index_range Perm>
constexpr auto compute_inertia_tensor(Points&& points, Perm&& perm)
    -> point_range_mat_t<Points> {
  TIT_ASSUME_UNIVERSAL(Points, points);
  TIT_ASSUME_UNIVERSAL(Perm, perm);
  return compute_inertia_tensor(permuted_points(points, perm));
}
/// @}

/// Try to compute the "largest" principal inertia axis.
///
/// By "largest" we mean the axis corresponding to the largest eigenvalue.
/// @{
template<point_range Points>
constexpr auto compute_largest_inertia_axis(Points&& points)
    -> std::expected<point_range_vec_t<Points>, MatEigError> {
  TIT_ASSUME_UNIVERSAL(Points, points);
  const auto inertia_tensor = compute_inertia_tensor(points);
  return jacobi(inertia_tensor).transform([](const auto& eig) {
    const auto& [V, d] = eig;
    return V[max_value_index(d)];
  });
}
template<point_range Points, index_range Perm>
constexpr auto compute_largest_inertia_axis(Points&& points, Perm&& perm)
    -> std::expected<point_range_vec_t<Points>, MatEigError> {
  TIT_ASSUME_UNIVERSAL(Points, points);
  TIT_ASSUME_UNIVERSAL(Perm, perm);
  return compute_largest_inertia_axis(permuted_points(points, perm));
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
