/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/containers/multivector.hpp"
#include "tit/core/math.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"
#include "tit/geom/point_range.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Uniform multidimensional grid spatial search index.
template<point_range Points>
  requires std::ranges::view<Points>
class GridIndex final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Point type.
  using Vec = std::ranges::range_value_t<Points>;

  /// Index the points for search using a grid.
  ///
  /// @param size_hint Grid cell size hint, typically 2x of the particle
  ///                  spacing.
  GridIndex(Points points, vec_num_t<Vec> size_hint)
      : points_{std::move(points)} {
    TIT_ASSERT(size_hint > 0.0, "Cell size hint must be positive!");
    // NOLINTBEGIN(*-prefer-member-initializer)

    // Compute bounding box.
    box_ = compute_bbox(points_);
    box_.grow(size_hint / 2);

    // Compute number of cells and cell sizes.
    const auto extents = box_.extents();
    const auto num_cells_float = maximum(ceil(extents / size_hint), Vec(1));
    num_cells_ = vec_cast<size_t>(num_cells_float);
    cell_extents_ = extents / num_cells_float;
    cell_extents_recip_ = Vec(1) / cell_extents_;

    // Pack the points into a multivector.
    cell_points_.assign_pairs_par_tall(
        prod(num_cells_),
        iota_perm(points_) | std::views::transform([this](size_t point) {
          const auto cell = cell_index_(points_[point]);
          return std::pair{flat_cell_index_(cell), point};
        }));

    // NOLINTEND(*-prefer-member-initializer)
  }

  /// Find the points within the radius to the given point.
  template<std::output_iterator<size_t> OutIter>
  auto search(const Vec& search_point,
              vec_num_t<Vec> search_radius,
              OutIter out) const -> OutIter {
    TIT_ASSERT(search_radius > 0.0, "Search radius should be positive.");

    // Calculate the search box.
    const auto half_cell_extents = cell_extents_ / 2;
    const auto search_box = BBox{search_point}
                                .grow(search_radius)
                                .grow(half_cell_extents)
                                .intersect(box_)
                                .shrink(half_cell_extents);
    const auto low = cell_index_(search_box.low());
    const auto high = cell_index_(search_box.high());

    // Collect points within the search box.
    const auto collect_cell_points = [&out,
                                      &search_point,
                                      search_dist = pow2(search_radius),
                                      this](const VecIndex_& cell_index) {
      out = copy_points_near(points_,
                             cell_points_[flat_cell_index_(cell_index)],
                             out,
                             search_point,
                             search_dist);
    };
    if constexpr (vec_dim_v<Vec> == 1) {
      for (size_t i = low[0]; i <= high[0]; ++i) {
        collect_cell_points({i});
      }
    } else if constexpr (vec_dim_v<Vec> == 2) {
      for (size_t i = low[0]; i <= high[0]; ++i) {
        for (size_t j = low[1]; j <= high[1]; ++j) {
          collect_cell_points({i, j});
        }
      }
    } else if constexpr (vec_dim_v<Vec> == 3) {
      for (size_t i = low[0]; i <= high[0]; ++i) {
        for (size_t j = low[1]; j <= high[1]; ++j) {
          for (size_t k = low[2]; k <= high[2]; ++k) {
            collect_cell_points({i, j, k});
          }
        }
      }
    } else static_assert(false);

    return out;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  // Index type.
  using VecIndex_ = decltype(vec_cast<size_t>(std::declval<Vec>()));

  // Compute the index of the cell containing the given point.
  auto cell_index_(const Vec& point) const -> VecIndex_ {
    const auto& origin = box_.low();
    const auto index_float = (point - origin) * cell_extents_recip_;
    TIT_ASSERT(index_float >= Vec(0), "Point is out of range!");
    TIT_ASSERT(index_float < vec_cast<vec_num_t<Vec>>(num_cells_),
               "Point is out of range!");
    return vec_cast<size_t>(index_float);
  }

  // Compute the flat index of the cell containing the given index.
  auto flat_cell_index_(const VecIndex_& index) const noexcept -> size_t {
    auto flat_index = index[0];
    for (size_t i = 1; i < vec_dim_v<Vec>; ++i) {
      flat_index = num_cells_[i] * flat_index + index[i];
    }
    return flat_index;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Points points_;
  BBox<Vec> box_;
  VecIndex_ num_cells_;
  Vec cell_extents_;
  Vec cell_extents_recip_;
  Multivector<size_t> cell_points_;

}; // class GridIndex

// Wrap a viewable range into a view on construction.
template<std::ranges::viewable_range Points, class... Args>
GridIndex(Points&&, Args...) -> GridIndex<std::views::all_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Grid based spatial search indexing function.
class GridSearch final {
public:

  /// Construct a grid search indexing function.
  ///
  /// @param size_hint Grid cell size, typically 2x of the particle spacing.
  constexpr explicit GridSearch(real_t size_hint) : size_hint_{size_hint} {
    TIT_ASSERT(size_hint_ > 0.0, "Cell size hint must be positive!");
  }

  /// Index the points for search using a grid.
  template<std::ranges::viewable_range Points>
    requires deduce_constructible_from<GridIndex, Points&&, real_t>
  [[nodiscard]] auto operator()(Points&& points) const {
    TIT_PROFILE_SECTION("GridSearch::operator()");
    return GridIndex{std::forward<Points>(points), size_hint_};
  }

private:

  real_t size_hint_;

}; // class GridSearch

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
