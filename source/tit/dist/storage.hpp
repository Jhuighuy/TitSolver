/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <optional>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/float.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/serialization.hpp"
#include "tit/data/storage.hpp"
#include "tit/mpi/mpi.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::dist {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Write a distributed particle array into a series.
///
/// The owned particles of all the processes are gathered onto the main
/// process and written in the deterministic canonical order — by particle
/// type first, then by the global identifier — which coincides with the
/// order a single-process run would produce. Only the main process may hold
/// the series; the remaining processes pass an empty optional.
template<sph::particle_array PA>
void write_frame(const PA& particles,
                 float64_t time,
                 std::optional<data::SeriesView<data::Storage>> series,
                 mpi::Comm comm = mpi::world) {
  TIT_PROFILE_SECTION("dist::write_frame()");

  // The single-process path matches the serial writer bit-for-bit.
  if (comm.size() == 1) {
    TIT_ASSERT(series.has_value(), "The main process must hold the series.");
    particles.write(time, *series);
    return;
  }
  TIT_ASSERT(series.has_value() == comm.is_main(),
             "Exactly the main process must hold the series.");

  // Gather the canonical global order: the owned particles of each process
  // are already sorted by type and identifier, so the global order is a
  // merge over the processes.
  const auto fluid_counts =
      comm.all_gather(std::ranges::size(particles.fluid()));
  std::vector<std::uint64_t> gids{};
  gids.reserve(particles.num_owned());
  for (const auto a : particles.owned()) gids.push_back(gid[a]);
  const auto [gid_bytes, gid_sizes] =
      comm.all_gather_v(std::as_bytes(std::span{gids}));
  const auto num_rows = gid_bytes.size() / sizeof(std::uint64_t);
  std::vector<std::uint64_t> all_gids{};
  std::vector<bool> row_is_fixed{};
  all_gids.reserve(num_rows);
  row_is_fixed.reserve(num_rows);
  for (const auto [rank, sizes] : std::views::enumerate(gid_sizes)) {
    const auto rank_rows = sizes / sizeof(std::uint64_t);
    const auto rank_offset = all_gids.size();
    for (const auto row : std::views::iota(std::size_t{0}, rank_rows)) {
      all_gids.push_back(from_bytes<std::uint64_t>(std::span{gid_bytes}.subspan(
          (rank_offset + row) * sizeof(std::uint64_t),
          sizeof(std::uint64_t))));
      row_is_fixed.push_back(row >=
                             fluid_counts[static_cast<std::size_t>(rank)]);
    }
  }

  // Compute the canonical permutation of the gathered rows: fluid particles
  // of all the processes ordered by the identifier, then the fixed ones.
  std::vector<std::size_t> order(num_rows);
  std::ranges::iota(order, std::size_t{0});
  std::ranges::sort(order, {}, [&all_gids, &row_is_fixed](std::size_t row) {
    return std::pair{row_is_fixed[row], all_gids[row]};
  });

  // Gather and write the fields.
  auto frame = series.has_value() ? std::optional{series->create_frame(time)} :
                                    std::nullopt;
  PA::varying_fields.for_each([&comm, &frame, &order, &particles](auto field) {
    if constexpr (!infra_fields.contains(decltype(field){})) {
      const auto column = field[particles].first(particles.num_owned());
      using Val = std::ranges::range_value_t<decltype(column)>;
      const auto [bytes, _] = comm.all_gather_v(std::as_bytes(column));
      if (!frame.has_value()) return;
      std::vector<Val> values{};
      values.reserve(order.size());
      for (const auto row : order) {
        values.push_back(from_bytes<Val>(
            std::span{bytes}.subspan(row * sizeof(Val), sizeof(Val))));
      }
      const auto array = frame->create_array(field.field_name);
      array.write(std::span<const Val>{values});
    }
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::dist
