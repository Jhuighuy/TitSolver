/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
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
#include "tit/core/par/algorithms.hpp"
#include "tit/core/range_utils.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"

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

/// Point range bounding box type.
template<point_range Points>
using point_range_bbox_t = BBox<point_range_vec_t<Points>>;

/// Point range dimension.
template<point_range Points>
inline constexpr auto point_range_dim_v = vec_dim_v<point_range_vec_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Count the number of points in the given range as a point range number type.
template<point_range Points>
constexpr auto count_points(Points&& points) -> point_range_num_t<Points> {
  TIT_ASSUME_UNIVERSAL(Points, points);
  return static_cast<point_range_num_t<Points>>(std::size(points));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compute the centroid of the given non-empty point range.
/// @{
template<point_range Points>
constexpr auto compute_center(Points&& points) -> point_range_vec_t<Points> {
  TIT_ASSUME_UNIVERSAL(Points, points);
  TIT_ASSERT(!std::ranges::empty(points), "Points must not be empty!");
  const auto sum = par::fold(par::static_, points);
  return sum / count_points(points);
}
template<point_range Points, index_range Perm>
constexpr auto compute_center(Points&& points, Perm&& perm)
    -> point_range_vec_t<Points> {
  TIT_ASSUME_UNIVERSAL(Points, points);
  TIT_ASSUME_UNIVERSAL(Perm, perm);
  return compute_center(permuted_view(points, perm));
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compute the bounding box of the given non-empty point range.
/// @{
template<point_range Points>
constexpr auto compute_bbox(Points&& points) -> point_range_bbox_t<Points> {
  TIT_ASSUME_UNIVERSAL(Points, points);
  /// @todo Assertion below fails to compile with GCC 14 in coverage mode.
#if !defined(TIT_HAVE_GCOV) || !TIT_HAVE_GCOV
  TIT_ASSERT(!std::ranges::empty(points), "Points must not be empty!");
#endif
  using Box = point_range_bbox_t<Points>;
  return par::fold(
      par::static_,
      points,
      Box{*std::begin(points)},
      [](Box partial, const auto& point) { return partial.expand(point); },
      [](Box partial, const auto& other) { return partial.join(other); });
}
template<point_range Points, index_range Perm>
constexpr auto compute_bbox(Points&& points, Perm&& perm)
    -> point_range_bbox_t<Points> {
  TIT_ASSUME_UNIVERSAL(Points, points);
  TIT_ASSUME_UNIVERSAL(Perm, perm);
  return compute_bbox(permuted_view(points, perm));
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compute the inertia tensor of the given non-empty point range.
///
/// Inertia tensor is defined as ∑rᵢ⊗rᵢ, where rᵢ is the position vector of
/// the i-th point relative to the center of mass.
/// @{
template<point_range Points>
constexpr auto compute_inertia_tensor(Points&& points)
    -> point_range_mat_t<Points> {
  TIT_ASSUME_UNIVERSAL(Points, points);
  TIT_ASSERT(!std::ranges::empty(points), "Points must not be empty!");
  struct Result {
    point_range_vec_t<Points> sum;
    point_range_mat_t<Points> tensor;
  };
  auto [sum, inertia_tensor] = par::fold(
      par::static_,
      points,
      Result{},
      [](Result partial, const auto& point) -> Result {
        return {partial.sum + point, partial.tensor + outer_sqr(point)};
      },
      [](Result partial, const Result& other) -> Result {
        return {partial.sum + other.sum, partial.tensor + other.tensor};
      });
  const auto center = sum / count_points(points);
  inertia_tensor -= outer(sum, center);
  return inertia_tensor;
}
template<point_range Points, index_range Perm>
constexpr auto compute_inertia_tensor(Points&& points, Perm&& perm)
    -> point_range_mat_t<Points> {
  TIT_ASSUME_UNIVERSAL(Points, points);
  TIT_ASSUME_UNIVERSAL(Perm, perm);
  return compute_inertia_tensor(permuted_view(points, perm));
}
/// @}

/// Try to compute the "largest" principal inertia axis of the given non-empty
/// point range.
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
  return compute_largest_inertia_axis(permuted_view(points, perm));
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Copy points that are close to the given point.
template<point_range Points,
         index_range Perm,
         std::output_iterator<size_t> OutIter,
         std::predicate<size_t> Pred = AlwaysTrue>
constexpr auto copy_points_near(Points&& points,
                                Perm&& perm,
                                OutIter out,
                                const point_range_vec_t<Points>& search_point,
                                point_range_num_t<Points> r_sqr,
                                Pred pred = {}) -> OutIter {
  TIT_ASSUME_UNIVERSAL(Points, points);
  TIT_ASSUME_UNIVERSAL(Perm, perm);
  const auto result = std::ranges::copy_if(
      perm,
      out,
      [&points, &search_point, r_sqr, &pred](size_t i) {
        return pred(i) && norm2(points[i] - search_point) < r_sqr;
      });
  return result.out;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
