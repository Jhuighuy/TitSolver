/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <ranges>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/range.hpp"
#include "tit/core/utils.hpp"

#include "tit/geom/point_range.hpp"
#include "tit/geom/sort.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Spatial sort based partitioning function.
template<sort_func Sort>
class SortPartition final {
public:

  /// Construct a spatial sort based partitioning function.
  /// @{
  constexpr SortPartition() = default;
  constexpr explicit SortPartition(Sort sort) noexcept
      : sort_{std::move(sort)} {}
  /// @}

  /// Partition the points using the spatial sort algorithm.
  template<point_range Points, output_index_range Parts>
  void operator()(Points&& points,
                  Parts&& parts,
                  size_t num_parts,
                  size_t init_part = 0) const {
    TIT_PROFILE_SECTION("SortPartition::operator()");
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

    // Build the permutation using the spatial sort.
    const auto num_points = std::size(points);
    std::vector<size_t> perm(num_points);
    sort_(points, perm);

    // Assign the partitions.
    const auto part_size = num_points / num_parts;
    const auto remainder = num_points % num_parts;
    for (size_t part = 0; part < num_parts; ++part) {
      const auto first = part * part_size + std::min(part, remainder);
      const auto last = (part + 1) * part_size + std::min(part + 1, remainder);
      for (size_t i = first; i < last; ++i) parts[perm[i]] = init_part + part;
    }
  }

private:

  [[no_unique_address]] Sort sort_;

}; // class SortPartition

/// Spatial sort based partitioning.
template<sort_func Sort>
inline constexpr SortPartition<Sort> sort_partition{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Hilbert curve sort based partitioning function.
using HilbertCurvePartition = SortPartition<HilbertCurveSort>;

/// Hilbert curve sort based partitioning.
inline constexpr HilbertCurvePartition hilbert_curve_partition{};

/// Morton curve sort based partitioning function.
using MortonCurvePartition = SortPartition<MortonCurveSort>;

/// Morton curve sort based partitioning.
inline constexpr MortonCurvePartition morton_curve_partition{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
