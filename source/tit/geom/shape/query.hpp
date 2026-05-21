/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"
#include "tit/geom/grid.hpp"
#include "tit/geom/shape/shape.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Shape query: point containment and sphere-face range query.
///
/// Given a preprocessed shape, provides fast point containment testing
/// (ray casting with even-odd rule) and sphere–face intersection queries,
/// both accelerated by a uniform grid.
template<class Vec>
class ShapeQuery final {
public:

  /// Numeric type.
  using Num = vec_num_t<Vec>;

  /// Dimension.
  static constexpr auto Dim = vec_dim_v<Vec>;

  /// Build the acceleration structure for the given shape.
  explicit ShapeQuery(const Shape<Vec>& shape) : shape_{&shape} {
    // Compute shape bounding box.
    for (const auto& v : shape_->verts()) bbox_.expand(v);
    if (shape_->num_verts() == 0) bbox_ = BBox<Vec>{};

    // If the shape has no faces, there is nothing to accelerate.
    if (shape_->num_faces() == 0) return;

    // Compute average face diameter for grid cell size.
    Num avg_diam{};
    for (std::size_t i = 0; i < shape_->num_faces(); ++i) {
      const auto fv = shape_->face_verts(i);
      const auto& v0 = shape_->vert(fv[0]);
      Num max_dist2{};
      for (std::size_t d = 1; d < Dim; ++d) {
        max_dist2 = std::max(max_dist2, norm2(shape_->vert(fv[d]) - v0));
      }
      avg_diam += sqrt(max_dist2);
    }
    avg_diam /= static_cast<Num>(shape_->num_faces());

    // Build the grid.
    const auto cell_extent = std::max(avg_diam / Num{4}, tiny_v<Num>);
    auto grid_bbox = bbox_;
    // Ensure non-zero extent in all dimensions (e.g. a 2D segment has
    // zero Y extent, a planar triangle has zero Z extent).
    grid_bbox.grow(cell_extent / Num{4});
    grid_ = Grid<Vec>{grid_bbox}.set_cell_extents(Vec(cell_extent));

    const auto num_cells = grid_.flat_num_cells();

    // Count faces per cell.
    std::vector<std::size_t> cell_counts(num_cells, 0);
    for (std::size_t i = 0; i < shape_->num_faces(); ++i) {
      for (const auto& cell : grid_.cells_intersecting(shape_->face(i).box())) {
        cell_counts[grid_.flatten_cell_index(cell)]++;
      }
    }

    // Build prefix-sum offsets and fill face indices.
    cell_offsets_.resize(num_cells + 1, 0);
    for (std::size_t i = 0; i < num_cells; ++i) {
      cell_offsets_[i + 1] = cell_offsets_[i] + cell_counts[i];
    }
    cell_faces_.resize(cell_offsets_[num_cells]);

    std::vector<std::size_t> fill_pos = cell_offsets_;
    for (std::size_t i = 0; i < shape_->num_faces(); ++i) {
      for (const auto& cell : grid_.cells_intersecting(shape_->face(i).box())) {
        cell_faces_[fill_pos[grid_.flatten_cell_index(cell)]++] = i;
      }
    }
  }

  /// Find all face indices that intersect the given sphere.
  auto query(const Vec& center, const Num& radius) const
      -> std::vector<std::size_t> {
    TIT_ASSERT(radius >= Num{0}, "Radius must be non-negative!");
    std::vector<std::size_t> result;
    if (shape_->num_faces() == 0) return result;

    // Quick rejection: sphere bounding box vs shape bounding box.
    const auto sphere_bbox = BBox<Vec>{center}.grow(radius);
    {
      bool overlap = true;
      for (std::size_t d = 0; d < Dim; ++d) {
        if (sphere_bbox.high()[d] < bbox_.low()[d] ||
            sphere_bbox.low()[d] > bbox_.high()[d]) {
          overlap = false;
          break;
        }
      }
      if (!overlap) return result;
    }

    std::vector<bool> visited(shape_->num_faces(), false);

    for (const auto& cell : grid_.cells_intersecting(sphere_bbox)) {
      const auto flat = grid_.flatten_cell_index(cell);
      for (auto idx = cell_offsets_[flat]; idx < cell_offsets_[flat + 1];
           ++idx) {
        const auto face_idx = cell_faces_[idx];
        if (visited[face_idx]) continue;
        visited[face_idx] = true;
        if (shape_->face(face_idx).intersects_sphere(center, radius)) {
          result.push_back(face_idx);
        }
      }
    }

    return result;
  }

private:

  const Shape<Vec>* shape_{};
  BBox<Vec> bbox_;
  Grid<Vec> grid_;
  std::vector<std::size_t> cell_offsets_;
  std::vector<std::size_t> cell_faces_;

}; // class ShapeQuery

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
