/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/vec.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Num>
class Domain final {
public:

  using Vec2 = Vec<Num, 2>;
  using Face = std::array<Vec2, 2>;

  constexpr Domain() = default;

  constexpr Domain(std::vector<Vec2> vertices,
                   Num max_face_length,
                   bool reverse = false)
      : vertices_{std::move(vertices)} {
    TIT_ASSERT(vertices_.size() >= 3, "Domain polygon must have 3+ vertices.");
    TIT_ASSERT(max_face_length > Num{0.0}, "Face length must be positive.");
    if (reverse) std::reverse(vertices_.begin(), vertices_.end());
    subdivide_faces_(max_face_length);
  }

  constexpr auto contains(const Vec2& point) const noexcept -> bool {
    bool inside = false;
    for (std::size_t i = 0, j = vertices_.size() - 1; i < vertices_.size();
         j = i++) {
      const auto& vi = vertices_[i];
      const auto& vj = vertices_[j];
      const bool intersects =
          ((vi[1] > point[1]) != (vj[1] > point[1])) &&
          (point[0] <
           (vj[0] - vi[0]) * (point[1] - vi[1]) / (vj[1] - vi[1]) + vi[0]);
      if (intersects) inside = !inside;
    }
    return inside;
  }

  constexpr auto num_faces() const noexcept -> std::size_t {
    return faces_.size();
  }

  constexpr auto face_nodes(std::size_t index) const noexcept {
    return std::array{index, (index + 1) % num_nodes()};
  }

  constexpr auto face(std::size_t index) const noexcept -> Face {
    TIT_ASSERT(index < faces_.size(), "Face index is out of range.");
    return faces_[index];
  }

  constexpr auto faces(std::size_t node_index) const noexcept {
    TIT_ASSERT(node_index < num_nodes(), "Node index is out of range.");
    return std::array{node_index, (node_index + num_nodes() - 1) % num_nodes()};
  }

  constexpr auto num_nodes() const noexcept -> std::size_t {
    return faces_.size();
  }

  constexpr auto node(std::size_t index) const noexcept -> Vec2 {
    TIT_ASSERT(index < faces_.size(), "Node index is out of range.");
    return faces_[index][0];
  }

private:

  constexpr void subdivide_faces_(Num max_face_length) {
    faces_.clear();
    for (std::size_t i = 0; i < vertices_.size(); ++i) {
      const auto& start = vertices_[i];
      const auto& end = vertices_[(i + 1) % vertices_.size()];
      const auto edge = end - start;
      const auto length = norm(edge);
      const auto num_segments = static_cast<std::size_t>(std::max(
          std::int64_t{1},
          static_cast<std::int64_t>(std::ceil(length / max_face_length))));
      for (std::size_t segment = 0; segment < num_segments; ++segment) {
        const auto t0 = static_cast<Num>(segment) / num_segments;
        const auto t1 = static_cast<Num>(segment + 1) / num_segments;
        faces_.push_back({start + t0 * edge, start + t1 * edge});
      }
    }
  }

  std::vector<Vec2> vertices_{};
  std::vector<Face> faces_{};

}; // class Domain

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
