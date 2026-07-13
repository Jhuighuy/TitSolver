/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <numeric>
#include <ranges>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bsphere.hpp"
#include "tit/geom/grid.hpp"
#include "tit/geom/point_range.hpp"
#include "tit/geom/surface.hpp"
#include "tit/par/algorithms.hpp"
#include "tit/par/atomic.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Uniform multidimensional grid spatial face search index.
template<class Vec>
  requires is_vec_v<Vec>
class GridFaceIndex final {
public:

  /// Temporary surfaces are not permitted.
  constexpr explicit GridFaceIndex(Surface<Vec>&&) = delete;

  /// Index the faces for search using a grid.
  ///
  /// @param size_hint Grid cell size.
  GridFaceIndex(const Surface<Vec>& surf, vec_num_t<Vec> size_hint)
      : surf_{&surf} {
    TIT_ASSERT(size_hint > 0.0, "Cell size hint must be positive!");

    // Early exit if the surface is empty.
    if (surf_->num_faces() == 0) return;
    const auto face_indices =
        std::views::iota(std::size_t{0}, surf_->num_faces());

    // Compute the bounding box of the surface.
    const auto box = compute_bbox(surf_->verts()).grow(size_hint / 2);
    grid_ = Grid{box}.set_cell_extents(size_hint);

    // Count the faces intersecting each cell. Counts are shifted by two so
    // that the fill positions turn into the final cell offsets in-place.
    cell_face_offsets_.resize(grid_.flat_num_cells() + 2);
    par::for_each(face_indices, [this](std::size_t face_index) {
      for (const auto& cell :
           grid_.cells_intersecting(surf_->face(face_index).box())) {
        const auto flat_cell = grid_.flatten_cell_index(cell);
        TIT_ASSERT(flat_cell < grid_.flat_num_cells(),
                   "Cell index is out of range!");
        par::fetch_and_add(cell_face_offsets_[flat_cell + 2], 1);
      }
    });

    // Convert the cell counts to offsets and allocate the face indices.
    std::partial_sum(cell_face_offsets_.begin() + 2,
                     cell_face_offsets_.end(),
                     cell_face_offsets_.begin() + 2);
    cell_faces_.resize(cell_face_offsets_.back());

    // Fill each cell range in parallel. Incrementing the shifted offsets
    // leaves the final CSR offsets in the first `num_cells + 1` entries.
    par::for_each(face_indices, [this](std::size_t face_index) {
      for (const auto& cell :
           grid_.cells_intersecting(surf_->face(face_index).box())) {
        const auto flat_cell = grid_.flatten_cell_index(cell);
        TIT_ASSERT(flat_cell < grid_.flat_num_cells(),
                   "Cell index is out of range!");
        const auto position =
            par::fetch_and_add(cell_face_offsets_[flat_cell + 1], 1);
        cell_faces_[position] = face_index;
      }
    });
    cell_face_offsets_.pop_back();
  }

  /// Find the faces intersecting the given sphere.
  template<std::output_iterator<std::size_t> OutIter>
  auto search(const BSphere<Vec>& search_sphere, OutIter out) const -> OutIter {
    if (surf_->num_faces() == 0) return out;
    auto& scratch = search_scratch_();
    const auto generation = scratch.start_search(surf_->num_faces());
    for (const auto& cell : grid_.cells_intersecting(search_sphere.box())) {
      const auto flat_cell = grid_.flatten_cell_index(cell);
      const auto first = cell_face_offsets_[flat_cell];
      const auto last = cell_face_offsets_[flat_cell + 1];
      for (const auto face_index :
           std::ranges::subrange(std::next(cell_faces_.begin(), first),
                                 std::next(cell_faces_.begin(), last))) {
        if (!scratch.mark_visited(face_index, generation)) continue;
        if (surf_->face(face_index).intersects(search_sphere)) {
          *out++ = face_index;
        }
      }
    }
    return out;
  }

private:

  class SearchScratch_ final {
  public:

    auto start_search(std::size_t num_faces) -> std::uint32_t {
      if (face_marks_.size() < num_faces) face_marks_.resize(num_faces);
      if (++generation_ == 0) {
        std::ranges::fill(face_marks_, 0);
        generation_ = 1;
      }
      return generation_;
    }

    auto mark_visited(std::size_t face_index, std::uint32_t generation)
        -> bool {
      TIT_ASSERT(face_index < face_marks_.size(),
                 "Face index is out of range!");
      auto& face_mark = face_marks_[face_index];
      if (face_mark == generation) return false;
      face_mark = generation;
      return true;
    }

  private:

    std::vector<std::uint32_t> face_marks_;
    std::uint32_t generation_ = 0;

  }; // class SearchScratch_

  static auto search_scratch_() -> SearchScratch_& {
    thread_local SearchScratch_ scratch;
    return scratch;
  }

  const Surface<Vec>* surf_;
  Grid<Vec> grid_;
  std::vector<std::size_t> cell_face_offsets_;
  std::vector<std::size_t> cell_faces_;

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
  /// Resulting index is valid only for the lifetime of the surface.
  template<class Vec>
    requires is_vec_v<Vec> && std::same_as<vec_num_t<Vec>, Num>
  [[nodiscard]] auto operator()(const Surface<Vec>& surf) const {
    TIT_PROFILE_SECTION("GridFaceSearch::operator()");
    return GridFaceIndex{surf, size_hint_};
  }

  /// Temporary surfaces are not permitted.
  template<class Vec>
  constexpr static auto operator()(Surface<Vec>&&) = delete;

private:

  Num size_hint_;

}; // class GridFaceSearch

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
