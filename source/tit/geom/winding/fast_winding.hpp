/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <inplace_vector>
#include <iterator>
#include <numeric>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/math.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"
#include "tit/geom/surface.hpp"
#include "tit/geom/winding/exact_winding.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Hierarchical fast generalized winding number for a surface.
template<class Vec>
  requires is_vec_v<Vec> && (vec_dim_v<Vec> == 2 || vec_dim_v<Vec> == 3)
class FastWindingFunc final {
public:

  /// Numeric type.
  using Num = vec_num_t<Vec>;

  /// Dimension of the surface.
  static constexpr auto Dim = vec_dim_v<Vec>;

  /// Temporary surfaces are not permitted.
  constexpr explicit FastWindingFunc(Surface<Vec>&&) = delete;

  /// Construct a fast winding number index for the surface.
  ///
  /// @param leaf_size Maximum number of exactly evaluated faces in a leaf.
  /// @param beta      Far-field acceptance ratio. Larger values are more
  ///                  accurate.
  constexpr explicit FastWindingFunc(const Surface<Vec>& surf,
                                     std::size_t leaf_size,
                                     Num beta)
      : surf_{&surf}, leaf_size_{leaf_size}, beta_{beta} {
    TIT_ASSERT(beta_ > Num{}, "Accuracy beta must be positive!");
    TIT_ASSERT(leaf_size_ > 0, "Leaf size must be positive!");
    if (surf_->num_faces() == 0) return;

    // Initialize the permutation.
    perm_.resize(surf_->num_faces());
    std::ranges::iota(perm_, std::size_t{0});

    // Initialize the node storage.
    nodes_.reserve(2 * (surf_->num_faces() / leaf_size_ + 1));

    // Recursively construct the tree.
    build_node_(perm_);
  }

  /// Estimate the generalized winding number at the point.
  constexpr auto operator()(const Vec& point) const noexcept -> Num {
    return eval_fast_(point, beta_);
  }

  /// Test whether the winding number is above a threshold.
  ///
  /// Queries close to the threshold are repeated more accurately and finally
  /// fall back to the exact estimator if they remain uncertain.
  constexpr auto contains(const Vec& point,
                          Num threshold = Num{0.5},
                          Num uncertainty = Num{1e-3}) const noexcept -> bool {
    TIT_ASSERT(uncertainty >= Num{}, "Uncertainty must not be negative!");
    auto winding = eval_fast_(point, beta_);
    if (abs(winding - threshold) < uncertainty) {
      winding = eval_fast_(point, Num{2} * beta_);
      if (abs(winding - threshold) < uncertainty) {
        winding = eval_exact_(point);
      }
    }
    return winding > threshold;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  using Moment0_ = std::array<Num, Dim>;      // First derivatives.
  using Moment1_ = std::array<Moment0_, Dim>; // Second derivatives.
  using Moment2_ = std::array<Moment1_, Dim>; // Third derivatives.

  // Face aggregate data.
  struct Aggregate_ final {
    BBox<Vec> box;
    Num weight{};
    Vec weighted_center{};
    Moment0_ moment_0{};
    Moment1_ moment_1{};
    Moment2_ moment_2{};
  };

  // Winding tree node data.
  struct Node_ final {
    Vec center{};
    Num radius_sqr{};
    Moment0_ moment_0{};
    Moment1_ moment_1{};
    Moment2_ moment_2{};
    std::size_t left = 0;
    std::size_t right = 0;
    std::size_t first = 0;
    std::size_t last = 0;
  };

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Build a winding tree node.
  /// @todo Parallelize the algorithm.
  constexpr auto build_node_(std::span<std::size_t> perm)
      -> std::pair<std::size_t, Aggregate_> {
    TIT_ASSERT(!perm.empty(), "Cannot build an empty winding node!");
    // Allocate the node.
    const auto node_index = nodes_.size();
    nodes_.emplace_back();
    auto& node = nodes_[node_index];

    Aggregate_ agg;
    if (perm.size() <= leaf_size_) {
      // Construct a leaf node.
      agg = aggregate_faces_(perm);
      node.first = static_cast<std::size_t>(perm.data() - perm_.data());
      node.last = node.first + perm.size();
    } else {
      // Split the current faces by the median.
      /// @todo Right now, splitting logic is inlined here. Once the amount of
      ///       our face algorithms grows, consider a possibility to introduce
      ///       analogues of `point_range.hpp` header and extension of
      ///       `bipartition.hpp` to support face ranges.
      BBox center_box{surf_->face(perm.front()).center()};
      for (const auto face_index : perm.subspan(1)) {
        center_box.expand(surf_->face(face_index).center());
      }
      const auto axis = max_value_index(center_box.extents());
      const auto middle_index = perm.size() / 2;
      const auto middle =
          std::next(perm.begin(), static_cast<std::ptrdiff_t>(middle_index));
      std::ranges::nth_element(perm, middle, {}, [this, axis](std::size_t a) {
        return surf_->face(a).center()[axis];
      });
      const auto left_perm = perm.first(middle_index);
      const auto right_perm = perm.subspan(middle_index);

      // Recursively build the subtrees.
      auto [left, left_aggregate] = build_node_(left_perm);
      auto [right, right_aggregate] = build_node_(right_perm);

      // Construct an interior node.
      agg = combine_aggregates_(left_aggregate, right_aggregate);
      nodes_[node_index].left = left;
      nodes_[node_index].right = right;
    }

    // Fill the aggregate data and return the node.
    init_node_(nodes_[node_index], agg);
    return {node_index, agg};
  }

  // Aggregate a set of faces.
  constexpr auto aggregate_faces_(std::span<const std::size_t> indices) const
      -> Aggregate_ {
    TIT_ASSERT(!indices.empty(), "Cannot aggregate an empty set of faces!");
    auto result = make_aggregate_(surf_->face(indices.front()));
    for (const auto face_index : indices.subspan(1)) {
      result =
          combine_aggregates_(result, make_aggregate_(surf_->face(face_index)));
    }
    return result;
  }

  // Aggregate a single face.
  static constexpr auto make_aggregate_(const Surface<Vec>::Face& face)
      -> Aggregate_ {
    const auto& [... verts] = face.verts();
    const auto center = face.center();
    const auto weighted_normal = face.wnormal();

    Aggregate_ result;

    result.box = face.box();

    result.weight = norm(weighted_normal);
    result.weighted_center = result.weight * center;

    for (std::size_t i = 0; i < Dim; ++i) {
      result.moment_0[i] = weighted_normal[i];
    }

    for (std::size_t i = 0; i < Dim; ++i) {
      for (std::size_t j = 0; j < Dim; ++j) {
        result.moment_1[i][j] = weighted_normal[j] * center[i];
      }
    }

    for (std::size_t i = 0; i < Dim; ++i) {
      for (std::size_t j = 0; j < Dim; ++j) {
        for (std::size_t k = 0; k < Dim; ++k) {
          result.moment_2[i][j][k] = weighted_normal[k] *
                                     ((verts[i] + ...) * (verts[j] + ...) +
                                      ((verts[i] * verts[j]) + ...)) /
                                     (Dim * (Dim + 1));
        }
      }
    }

    return result;
  }

  // Combine two aggregates into a single one.
  static constexpr auto combine_aggregates_(const Aggregate_& a,
                                            const Aggregate_& b) -> Aggregate_ {
    Aggregate_ result;

    result.box = BBox{a.box}.join(b.box);

    result.weight = a.weight + b.weight;
    result.weighted_center = a.weighted_center + b.weighted_center;

    for (std::size_t i = 0; i < Dim; ++i) {
      result.moment_0[i] = a.moment_0[i] + b.moment_0[i];
    }

    for (std::size_t i = 0; i < Dim; ++i) {
      for (std::size_t j = 0; j < Dim; ++j) {
        result.moment_1[i][j] = a.moment_1[i][j] + b.moment_1[i][j];
      }
    }

    for (std::size_t i = 0; i < Dim; ++i) {
      for (std::size_t j = 0; j < Dim; ++j) {
        for (std::size_t k = 0; k < Dim; ++k) {
          result.moment_2[i][j][k] = a.moment_2[i][j][k] + b.moment_2[i][j][k];
        }
      }
    }

    return result;
  }

  // Initialize a node from the aggregate data.
  static constexpr void init_node_(Node_& node, const Aggregate_& agg) {
    node.center = agg.weight > Num{} ? agg.weighted_center / agg.weight :
                                       agg.box.center();

    node.radius_sqr = norm2(maximum(abs(node.center - agg.box.low()),
                                    abs(agg.box.high() - node.center)));

    node.moment_0 = agg.moment_0;

    for (std::size_t i = 0; i < Dim; ++i) {
      for (std::size_t j = 0; j < Dim; ++j) {
        node.moment_1[i][j] =
            agg.moment_1[i][j] - node.center[i] * agg.moment_0[j];
      }
    }

    for (std::size_t i = 0; i < Dim; ++i) {
      for (std::size_t j = 0; j < Dim; ++j) {
        for (std::size_t k = 0; k < Dim; ++k) {
          node.moment_2[i][j][k] =
              agg.moment_2[i][j][k] - node.center[i] * agg.moment_1[j][k] -
              node.center[j] * agg.moment_1[i][k] +
              node.center[i] * node.center[j] * agg.moment_0[k];
        }
      }
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Evaluate the exact winding number at the point.
  constexpr auto eval_exact_(const Vec& point) const noexcept -> Num {
    return make_exact_winding(*surf_)(point);
  }

  // Evaluate the fast winding number at the point.
  constexpr auto eval_fast_(const Vec& point, Num beta) const noexcept -> Num {
    if (nodes_.empty()) return Num{};
    Num result{};
    const auto beta_sqr = pow2(beta);
    for (std::inplace_vector<std::uint32_t, 64> stack{0}; !stack.empty();) {
      const auto& node = nodes_[stack.back()];
      stack.pop_back();
      const auto delta = node.center - point;
      if (const auto dist_sqr = norm2(delta);
          dist_sqr > beta_sqr * node.radius_sqr) {
        result += eval_expansion_(delta,
                                  dist_sqr,
                                  node.moment_0,
                                  node.moment_1,
                                  node.moment_2);
      } else if (node.first != node.last) {
        for (const auto i : std::views::iota(node.first, node.last)) {
          result += surf_->face(perm_[i]).winding_number(point);
        }
      } else {
        stack.push_back(node.right);
        stack.push_back(node.left);
      }
    }
    return result;
  }

  // Evaluate the expansion of the winding number at the point.
  static constexpr auto eval_expansion_(const Vec& delta,
                                        Num dist_sqr,
                                        const Moment0_& moment_0,
                                        const Moment1_& moment_1,
                                        const Moment2_& moment_2) noexcept
      -> Num {
    constexpr Num scale{inverse(unit_sphere_area_v<Dim>)};
    const auto inv_dist_sqr = inverse(dist_sqr);
    const auto inv_dist_dim = pow(sqrt(inv_dist_sqr), Dim);

    Num result{};

    for (std::size_t i = 0; i < Dim; ++i) {
      const auto gradient = scale * inv_dist_dim * delta[i];
      result += moment_0[i] * gradient;
    }

    for (std::size_t i = 0; i < Dim; ++i) {
      for (std::size_t j = 0; j < Dim; ++j) {
        const auto identity = i == j ? Num{1} : Num{};
        const auto hessian =
            scale * inv_dist_dim *
            (identity - Dim * delta[i] * delta[j] * inv_dist_sqr);
        result += moment_1[i][j] * hessian;
      }
    }

    for (std::size_t i = 0; i < Dim; ++i) {
      for (std::size_t j = 0; j < Dim; ++j) {
        for (std::size_t k = 0; k < Dim; ++k) {
          const auto identity_terms = (i == j ? delta[k] : Num{}) +
                                      (i == k ? delta[j] : Num{}) +
                                      (j == k ? delta[i] : Num{});
          const auto third_derivative =
              Dim * scale * inv_dist_dim * inv_dist_sqr *
              (((Dim + 2) * delta[i] * delta[j] * delta[k] * inv_dist_sqr) -
               identity_terms);
          result += Num{0.5} * moment_2[i][j][k] * third_derivative;
        }
      }
    }

    return result;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  const Surface<Vec>* surf_;
  std::size_t leaf_size_;
  Num beta_;
  std::vector<std::size_t> perm_;
  std::vector<Node_> nodes_;

}; // class FastWindingFunc

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fast generalized winding number function constructor.
template<class Num>
class MakeFastWinding final {
public:

  /// Initialize the fast winding number function constructor.
  ///
  /// @param leaf_size Maximum number of exactly evaluated faces in a leaf.
  /// @param beta      Far-field acceptance ratio. Larger values are more
  ///                  accurate.
  constexpr explicit MakeFastWinding(std::size_t leaf_size = 8,
                                     Num beta = Num{2})
      : leaf_size_{leaf_size}, beta_{beta} {}

  /// Construct a fast winding number function for the surface.
  template<class Vec>
    requires is_vec_v<Vec> && std::same_as<vec_num_t<Vec>, Num>
  [[nodiscard]] auto operator()(const Surface<Vec>& surf) const {
    TIT_PROFILE_SECTION("FastWindingIndexing::operator()");
    return FastWindingFunc{surf, leaf_size_, beta_};
  }

  /// Temporary surfaces are not permitted.
  template<class Vec>
  constexpr static auto operator()(Surface<Vec>&&) = delete;

private:

  std::size_t leaf_size_;
  Num beta_;

}; // class MakeFastWinding

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
