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
#include "tit/core/mat.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/vec.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Inertial bisection partitioning.
template<std::ranges::view Points, std::ranges::view Parts>
  requires (std::ranges::sized_range<Points> &&
            std::ranges::random_access_range<Points> &&
            is_vec_v<std::ranges::range_value_t<Points>>) &&
           (std::ranges::random_access_range<Parts> &&
            std::ranges::output_range<Parts, size_t>)
class InertialBisection final {
public:

  /// Initialize and build the partitioning.
  InertialBisection(Points points, Parts parts, size_t num_parts)
      : points_{std::move(points)}, parts_{std::move(parts)} {
    TIT_PROFILE_SECTION("InertialBisection::InertialBisection()");
    build_(num_parts);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  // Build the partitioning recursively.
  void build_(size_t num_parts) {
    // Initialize identity permutation.
    perm_.resize(std::size(points_));
    std::ranges::copy(std::views::iota(size_t{0}, perm_.size()), perm_.begin());

    // Build the partitioning.
    par::TaskGroup tasks{};
    partition_(tasks, num_parts, /*part_index=*/0, perm_);
    tasks.wait();
  }

  // Partition the points by bisecting the longest inertial axis.
  void partition_(par::TaskGroup& tasks,
                  size_t num_parts,
                  size_t part_index,
                  std::span<size_t> perm) {
    if (num_parts == 1) {
      // No further partitioning, assign part index to the output.
      for (const auto i : perm) parts_[i] = part_index;
      return;
    }

    // Compute the inertia tensor.
    //
    // Note: the true inertia tensor is ∑(rᵢ·rᵢI - rᵢ⊗rᵢ) where rᵢ is the
    // position vector of the i-th point relative to the center of mass.
    // Since the first term is a scalar multiple of the identity matrix, it
    // does not affect the eigenvectors of the inertia tensor. Thus, we can
    // simplify the computation to ∑(rᵢ⊗rᵢ) and seek for largest eigenvector
    // of this matrix instead of the smallest eigenvector of the true inertia
    // tensor.
    //
    /// @todo Introduce a helper function for this computation.
    auto sum = points_[perm[0]];
    auto inertia_tensor = outer_sqr(points_[perm[0]]);
    for (const auto i : perm.subspan(1)) {
      const auto& point = points_[i];
      sum += point;
      inertia_tensor += outer_sqr(point);
    }
    inertia_tensor -= outer(sum, sum / perm.size());

    // Compute the inertia axis corresponding to the smallest principal inertia
    // moment.
    const auto inertia_axis = //
        jacobi(inertia_tensor)
            // Axis of inertia is the largest eigenvector.
            .transform([](const auto& eig) {
              const auto& [V, d] = eig;
              return V[max_value_index(d)];
            })
            // Fallback to a unit vector as an axis of inertia if
            // the eigendecomposition fails.
            .value_or(unit(points_[0]));

    // Split the parts into halves.
    const auto left_num_parts = num_parts / 2;
    const auto right_num_parts = num_parts - left_num_parts;
    const auto left_part_index = part_index;
    const auto right_part_index = part_index + left_num_parts;

    // Partition the permutation along the inertial axis.
    const auto median = left_num_parts * perm.size() / num_parts;
    std::ranges::nth_element( //
        perm,
        perm.begin() + static_cast<ssize_t>(median),
        [&inertia_axis, this](size_t i, size_t j) {
          return dot(inertia_axis, points_[i] - points_[j]) < 0;
        });
    const auto left_range = perm.subspan(0, median);
    const auto right_range = perm.subspan(median);

    // Recursively partition the range.
    tasks.run([left_num_parts, left_part_index, left_range, &tasks, this] {
      partition_(tasks, left_num_parts, left_part_index, left_range);
    });
    tasks.run([right_range, right_num_parts, right_part_index, &tasks, this] {
      partition_(tasks, right_num_parts, right_part_index, right_range);
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Points points_;
  Parts parts_;
  std::vector<size_t> perm_;

}; // class InertialBisection

// Wrap a viewable range into a view on construction.
template<class Points, class Parts, class... Args>
InertialBisection(Points&&, Parts&&, Args...)
    -> InertialBisection<std::views::all_t<Points>, std::views::all_t<Parts>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
