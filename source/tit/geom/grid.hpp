/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <ranges>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Uniform multidimensional grid.
template<class Vec>
class Grid final {
public:

  // Bounding box type.
  using Box = BBox<Vec>;

  // Index type.
  using VecIndex = decltype(vec_cast<size_t>(std::declval<Vec>()));

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize a grid with an empty bounding box and zero cells.
  constexpr Grid() = default;

  /// Initialize a grid with the given bounding box and number of cells.
  constexpr explicit Grid(Box box, const VecIndex& num_cells = VecIndex(1))
      : box_{std::move(box)} {
    set_num_cells(num_cells);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Get the bounding box.
  constexpr auto box() const noexcept -> const Box& {
    return box_;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Get the number of cells.
  constexpr auto num_cells() const noexcept -> const VecIndex& {
    return num_cells_;
  }

  /// Get the flat number of cells.
  constexpr auto flat_num_cells() const noexcept -> size_t {
    return prod(num_cells_);
  }

  /// Set the number of cells.
  constexpr auto set_num_cells(const VecIndex& num_cells) -> Grid& {
    TIT_ASSERT(num_cells > VecIndex(0), "Number of cells must be positive!");
    num_cells_ = num_cells;
    cell_extents_ = box_.extents() / vec_cast<vec_num_t<Vec>>(num_cells_);
    inv_cell_extents_ = Vec(1) / cell_extents_;
    return *this;
  }

  /// Extend the number of cells by the given amount in each direction.
  constexpr auto extend(size_t amount) -> Grid& {
    TIT_ASSERT(amount > 0, "Amount must be positive!");
    num_cells_ += VecIndex(2 * amount);
    box_.grow(cell_extents_ * static_cast<vec_num_t<Vec>>(amount));
    return *this;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Get the cell extents.
  constexpr auto cell_extents() const noexcept -> const Vec& {
    return cell_extents_;
  }

  /// Set the number of cells from cell size hint.
  /// @{
  constexpr auto set_cell_extents(vec_num_t<Vec> size_hint) -> Grid& {
    TIT_ASSERT(size_hint > 0, "Cell size hint must be positive!");
    return set_cell_extents(Vec(size_hint));
  }
  constexpr auto set_cell_extents(const Vec& size_hint) -> Grid& {
    TIT_ASSERT(size_hint > Vec(0), "Cell size hint must be positive!");
    const auto extents = box_.extents();
    const auto num_cells_float = maximum(ceil(extents / size_hint), Vec(1));
    num_cells_ = vec_cast<size_t>(num_cells_float);
    cell_extents_ = extents / num_cells_float;
    inv_cell_extents_ = Vec(1) / cell_extents_;
    return *this;
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Index of the cell containing the given point:
  /// `low(cell) <= point < high(cell)`.
  constexpr auto cell_index(const Vec& point) const -> VecIndex {
    const auto& origin = box_.low();
    const auto index_float = (point - origin) * inv_cell_extents_;
    TIT_ASSERT(index_float >= Vec(0), "Point is out of range!");
    TIT_ASSERT(index_float < vec_cast<vec_num_t<Vec>>(num_cells_),
               "Point is out of range!");
    return vec_cast<size_t>(index_float);
  }

  /// Flat index of the cell containing the given point.
  constexpr auto flat_cell_index(const Vec& point) const -> size_t {
    return flatten_cell_index(cell_index(point));
  }

  /// Flat index of the cell containing the given index.
  constexpr auto flatten_cell_index(const VecIndex& index) const -> size_t {
    TIT_ASSERT(index < num_cells_, "Index is out of bounds!");
    auto flat_index = index[0];
    for (size_t i = 1; i < vec_dim_v<Vec>; ++i) {
      flat_index = num_cells_[i] * flat_index + index[i];
    }
    return flat_index;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Range of cell indices, such that `low <= index < high`.
  constexpr auto cells(const VecIndex& low,
                       const VecIndex& high) const noexcept {
    TIT_ASSERT(low <= high, "Invalid cell range!");
    TIT_ASSERT(high <= num_cells_, "Invalid cell range!");
    return [&]<size_t... Axes>(std::index_sequence<Axes...> /*axes*/) {
      return std::views::cartesian_product(
                 std::views::iota(low[Axes], high[Axes])...) |
             std::views::transform([](const auto& index_tuple) {
               return std::apply(
                   [](const auto&... indices) { return VecIndex{indices...}; },
                   index_tuple);
             });
    }(std::make_index_sequence<vec_dim_v<Vec>>{});
  }

  /// Range of cell indices, such that `low <= index <= high`.
  constexpr auto cells_inclusive(const VecIndex& low,
                                 const VecIndex& high) const noexcept {
    return cells(low, high + VecIndex(1));
  }

  /// Range of cell indices, such that `n <= index < num_cells - n`.
  constexpr auto cells(size_t n = 0) const noexcept {
    return cells(VecIndex(n), num_cells_ - VecIndex(n));
  }

  /// Range of cell indices that intersect the given search box.
  /// Search box **must** have a non-empty intersection with the grid.
  constexpr auto cells_intersecting(const Box& search_box) const noexcept {
    const auto half_cell_extents = cell_extents_ / 2;
    const auto safe_intersection = auto{search_box}
                                       .grow(half_cell_extents)
                                       .intersect(box_)
                                       .shrink(half_cell_extents);
    const auto low = cell_index(safe_intersection.low());
    const auto high = cell_index(safe_intersection.high());
    return cells_inclusive(low, high);
  }

private:

  Box box_;
  VecIndex num_cells_;
  Vec cell_extents_;
  Vec inv_cell_extents_;

}; // class Grid

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
