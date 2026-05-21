/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <unordered_set>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/mdvector.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bsphere.hpp"
#include "tit/geom/grid.hpp"
#include "tit/geom/point_range.hpp"
#include "tit/geom/surface.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Uniform multidimensional grid spatial face search index.
template<class Vec>
  requires is_vec_v<Vec>
class GridFaceIndex final {
public:

  /// Index the faces for search using a grid.
  GridFaceIndex(const Surface<Vec>& surface, vec_num_t<Vec> size_hint)
      : surface_{&surface} {
    TIT_ASSERT(size_hint > 0.0, "Cell size hint must be positive!");
    const auto box = compute_bbox(surface_->verts()).grow(size_hint / 2);
    grid_ = Grid{box}.set_cell_extents(size_hint);
    cell_faces_.assign(grid_.num_cells().elems());
    for (const auto& [face_index, face] :
         std::views::enumerate(surface_->faces())) {
      for (const auto cell : grid_.cells_intersecting(face.box())) {
        cell_faces_[cell.elems()].push_back(face_index);
      }
    }
  }

  /// Find the faces intersecting the given sphere.
  template<std::output_iterator<std::size_t> OutIter>
  auto search(const BSphere<Vec>& search_sphere, OutIter out) const -> OutIter {
    std::unordered_set<std::size_t> visited;
    for (const auto& cell : grid_.cells_intersecting(search_sphere.box())) {
      for (const auto face_index : cell_faces_[cell.elems()]) {
        if (visited.insert(face_index).second &&
            surface_->face(face_index).intersects(search_sphere)) {
          *out++ = face_index;
        }
      }
    }
    return out;
  }

private:

  const Surface<Vec>* surface_;
  Grid<Vec> grid_;
  Mdvector<std::vector<std::size_t>, vec_dim_v<Vec>> cell_faces_;

}; // class GridFaceIndex

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Grid-based spatial face search indexing function.
template<class Num>
class GridFaceSearch final {
public:

  /// Construct a grid face search indexing function.
  ///
  /// @param size_hint Grid cell size.
  constexpr explicit GridFaceSearch(Num size_hint) : size_hint_{size_hint} {
    TIT_ASSERT(size_hint_ > 0.0, "Cell size hint must be positive!");
  }

  /// Index the faces for search using a grid.
  template<class Vec>
    requires is_vec_v<Vec> && std::same_as<vec_num_t<Vec>, Num>
  [[nodiscard]] auto operator()(const Surface<Vec>& surface) const {
    TIT_PROFILE_SECTION("GridFaceSearch::operator()");
    return GridFaceIndex{surface, size_hint_};
  }

private:

  Num size_hint_;

}; // class GridFaceSearch

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
