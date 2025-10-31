/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <functional>
#include <numeric>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/range.hpp"
#include "tit/core/utils.hpp"
#include "tit/geom/bipartition.hpp"
#include "tit/geom/point_range.hpp"
#include "tit/par/task_group.hpp"

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
                  std::ranges::range_value_t<Parts> num_parts,
                  std::ranges::range_value_t<Parts> init_part = 0) const {
    TIT_PROFILE_SECTION("RecursiveBisection::operator()");
    TIT_ASSUME_UNIVERSAL(Points, points);
    TIT_ASSUME_UNIVERSAL(Parts, parts);

    // Validate the arguments.
    TIT_ASSERT(num_parts > 0, "Number of parts must be positive!");
    if constexpr (std::ranges::sized_range<Parts>) {
      TIT_ASSERT(std::size(points) == std::size(parts),
                 "Size of parts range must be equal to the number of points!");
    }

    // Initialize the permutation.
    std::vector<size_t> perm(std::ranges::size(points));
    std::ranges::iota(perm, size_t{0});

    // Partition the points.
    par::TaskGroup tasks{};
    const auto impl = [&points, &parts, &tasks, &bisection = this->bisection_](
                          this const auto& self,
                          std::ranges::range_value_t<Parts> my_num_parts,
                          std::ranges::range_value_t<Parts> my_part,
                          std::span<size_t> my_perm) {
      TIT_ASSERT(my_perm.size() >= my_num_parts,
                 "Number of points cannot be less than the number of parts!");
      if (my_num_parts == 1) {
        // No further partitioning, assign part index to the output.
        std::ranges::fill(permuted_view(parts, my_perm), my_part);
        return;
      }

      // Split the points into the roughly equal halves.
      const auto left_num_parts = my_num_parts / 2;
      const auto right_num_parts = my_num_parts - left_num_parts;
      const auto left_part = my_part;
      const auto right_part = my_part + left_num_parts;
      const auto median_index = left_num_parts * my_perm.size() / my_num_parts;
      const auto [left_perm, right_perm] =
          bisection(points, my_perm, median_index);

      // Recursively partition the halves.
      tasks.run(std::bind_front(self, left_num_parts, left_part, left_perm));
      tasks.run(std::bind_front(self, right_num_parts, right_part, right_perm));
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
