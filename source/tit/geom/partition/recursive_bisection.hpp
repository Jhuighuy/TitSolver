/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/utils.hpp"

#include "tit/geom/bipartition.hpp"
#include "tit/geom/utils.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Recursive bisection partitioning function.
template<class Bisection>
class RecursiveBisection final {
public:

  /// Construct a recursive bisection partitioning function.
  /// @{
  constexpr RecursiveBisection() = default;
  constexpr explicit RecursiveBisection(Bisection bisection) noexcept
      : bisection_{std::move(bisection)} {}
  /// @}

  /// Partition the points recursively using the bisector function.
  template<point_range Points, output_index_range Parts>
  void operator()(Points&& points,
                  Parts&& parts,
                  size_t num_parts,
                  size_t init_part = 0) const {
    TIT_PROFILE_SECTION("RecursiveBisection::operator()");
    TIT_ASSUME_UNIVERSAL(Points, points);
    TIT_ASSUME_UNIVERSAL(Parts, parts);

    // Validate the arguments.
    TIT_ASSERT(num_parts > 0, "Number of parts must be positive!");
    TIT_ASSERT(std::size(points) >= num_parts, "Number of parts is too large!");
    if constexpr (std::ranges::sized_range<Parts>) {
      TIT_ASSERT(std::size(points) == std::size(parts),
                 "Number of parts is different from the number of points!");
    }

    // Initialize the permutation.
    auto perm = std::views::iota(size_t{0}, std::size(points)) |
                std::ranges::to<std::vector>();

    // GCC has a bug with capturing `this` in a lambda with deduced this.
    auto* const this_ = this;

    // Partition the points.
    par::TaskGroup tasks{};
    const auto impl = [&points, &parts, &tasks, this_](
                          this const auto& self,
                          size_t my_num_parts,
                          size_t my_part_index,
                          std::span<size_t> my_perm) {
      TIT_ASSERT(!my_perm.empty(), "Permutation is empty!");
      if (my_num_parts == 1) {
        // No further partitioning, assign part index to the output.
        for (const auto i : my_perm) parts[i] = my_part_index;
        return;
      }

      // Prepare subranges for the halves.
      const auto left_num_parts = my_num_parts / 2;
      const auto right_num_parts = my_num_parts - left_num_parts;
      const auto left_part_index = my_part_index;
      const auto right_part_index = my_part_index + left_num_parts;
      const auto median_index = left_num_parts * my_perm.size() / my_num_parts;
      const auto left_perm = my_perm.subspan(0, median_index);
      const auto right_perm = my_perm.subspan(median_index);

      // Bisect the points.
      const auto median = my_perm.begin() + static_cast<ssize_t>(median_index);
      this_->bisection_(points, my_perm, median);

      // Recursively partition the halves.
      tasks.run([left_num_parts, left_part_index, left_perm, &self] {
        self(left_num_parts, left_part_index, left_perm);
      });
      tasks.run([right_perm, right_num_parts, right_part_index, &self] {
        self(right_num_parts, right_part_index, right_perm);
      });
    };
    impl(num_parts, init_part, perm);
    tasks.wait();
  }

private:

  [[no_unique_address]] Bisection bisection_;

}; // class RecursiveBisection

/// Recursive bisection partitioning.
template<class Bisection>
inline constexpr RecursiveBisection<Bisection> recursive_bisection{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Recursive coordinate bisection partitioning function.
using RecursiveCoordBisection = RecursiveBisection<CoordMedianSplit>;

/// Recursive coordinate bisection partitioning.
inline constexpr RecursiveCoordBisection recursive_coord_bisection{};

/// Recursive inertial bisection partitioning function.
using RecursiveInertialBisection = RecursiveBisection<InertialMedianSplit>;

/// Recursive inertial bisection partitioning.
inline constexpr RecursiveInertialBisection recursive_inertial_bisection{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
