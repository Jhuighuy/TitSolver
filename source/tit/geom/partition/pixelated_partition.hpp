/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// @todo Missing tests.
#pragma once

#include <array>
#include <concepts>
#include <deque>
#include <ranges>
#include <span>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/containers/mdvector.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/range.hpp"
#include "tit/core/utils.hpp"

#include "tit/geom/grid.hpp"
#include "tit/geom/point_range.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Num, class Partition>
class PixelatedPartition final {
public:

  /// Construct a pixelated partitioning function.
  ///
  /// @param size_hint Pixel size, typically 2x of the particle spacing.
  /// @param partition Partitioning function.
  constexpr explicit PixelatedPartition(Num size_hint,
                                        Partition partition) noexcept
      : size_hint_{size_hint}, partition_{partition} {
    TIT_ASSERT(size_hint_ > 0.0, "Cell size hint must be positive!");
  }

  /// Partition the points using the grid graph partitioning algorithm.
  template<point_range Points, output_index_range Parts>
    requires std::same_as<point_range_num_t<Points>, Num>
  void operator()(Points&& points,
                  Parts&& parts,
                  std::ranges::range_value_t<Parts> num_parts,
                  std::ranges::range_value_t<Parts> init_part = 0) const {
    TIT_PROFILE_SECTION("PixelatedPartition::operator()");
    TIT_ASSUME_UNIVERSAL(Points, points);
    TIT_ASSUME_UNIVERSAL(Parts, parts);

    // Validate the arguments.
    TIT_ASSERT(num_parts > 0, "Number of parts must be positive!");
    TIT_ASSERT(std::size(points) >= num_parts,
               "Number of points cannot be less than the number of parts!");
    if constexpr (std::ranges::sized_range<Parts>) {
      TIT_ASSERT(std::size(points) == std::size(parts),
                 "Size of parts range must be equal to the number of points!");
    }

    // Compute bounding box and initialize the pixel grid. We'll extend the
    // number of cells by one in each direction to avoid the conditionals near
    // boundary.
    const auto box = compute_bbox(points).grow(size_hint_ / 2);
    const auto grid = Grid{box}.set_cell_extents(size_hint_);

    // Identify the active pixels and collect their coordinates.
    struct Pixel {
      size_t index = npos;
      bool active = false;
    };
    Mdvector<Pixel, point_range_dim_v<Points>> pixels{grid.num_cells().elems()};
    for (const auto& point : points) {
      const auto pixel_coords = grid.cell_index(point);
      pixels[pixel_coords.elems()].active = true;
    }
    std::vector<point_range_vec_t<Points>> pixelated_points;
    for (const auto& pixel_coords : grid.cells()) {
      auto& [index, active] = pixels[pixel_coords.elems()];
      if (active) {
        index = pixelated_points.size();
        pixelated_points.push_back(
            vec_cast<point_range_num_t<Points>>(pixel_coords));
      }
    }

    // Partition the pixel grid using the partitioning function.
    std::vector<std::ranges::range_value_t<Parts>> pixelated_parts(
        pixelated_points.size());
    partition_(pixelated_points, pixelated_parts, num_parts, init_part);

    // Assign the final part indices.
    for (auto&& [point, part] : std::views::zip(points, parts)) {
      part = pixelated_parts[pixels[grid.cell_index(point).elems()].index];
    }
  }

private:

  Num size_hint_;
  [[no_unique_address]] Partition partition_;

}; // class PixelatedPartition

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
