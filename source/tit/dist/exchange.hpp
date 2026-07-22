/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <numeric>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/serialization.hpp"
#include "tit/geom/bsphere.hpp"
#include "tit/geom/partition.hpp"
#include "tit/geom/search.hpp"
#include "tit/geom/sort.hpp"
#include "tit/mpi/mpi.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::dist {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Distributed particle exchange.
///
/// The exchange decomposes the particle set geometrically across the
/// processes, maintains the ghost particle mirrors within the halo radius of
/// the subdomain boundaries, migrates the particle ownership as the
/// particles move, and refreshes the ghost field values between the
/// computational phases. It models the `tit::sph::exchange_for` interface.
///
/// The current implementation recomputes the decomposition redundantly on
/// every process from the gathered particle positions. This is simple and
/// robust, and acceptable up to a few million particles; a fully distributed
/// decomposition is planned as an optimization (see /MPI_MIGRATION.md).
///
/// Invariant: between the calls, the owned particles of every process are
/// ordered canonically — by type first, then by the global identifier. Both
/// sides of every exchange derive the message layouts independently from
/// this invariant, so no index lists ever need to be communicated.
///
/// @tparam PartitionFunc Geometric partitioning function. It must be
///                       deterministic: every process partitions the same
///                       gathered point set and must arrive at the same
///                       result. The default — spatial sort along the
///                       Hilbert curve — satisfies this: it performs exact
///                       coordinate comparisons only.
template<geom::partition_func PartitionFunc =
             geom::SortPartition<geom::HilbertCurveSort>>
class ParticleExchange final {
public:

  /// Construct a particle exchange.
  ///
  /// @param halo_radius Radius of the ghost particle halo. Must cover the
  ///                    maximal particle interaction radius, plus a margin
  ///                    ("skin") that covers the particle displacement
  ///                    within a single time step: the ghost membership is
  ///                    rebuilt between the steps only.
  /// @param partition_func Geometric partitioning function.
  /// @param comm Communicator to operate on.
  explicit ParticleExchange(double halo_radius,
                            PartitionFunc partition_func = {},
                            mpi::Comm comm = mpi::world)
      : halo_radius_{halo_radius}, partition_func_{std::move(partition_func)},
        comm_{comm} {
    TIT_ASSERT(halo_radius_ > 0.0, "Halo radius must be positive.");
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Distribute the initially replicated particles.
  ///
  /// Every process must enter with an identical, fully replicated particle
  /// array. Each process keeps the particles of its own subdomain, and the
  /// ghost mirrors are established. No communication is involved beyond the
  /// initial field refresh.
  template<sph::particle_array PA>
  void distribute(PA& particles) {
    TIT_PROFILE_SECTION("dist::ParticleExchange::distribute()");
    TIT_ASSERT(particles.num_owned() == particles.size(),
               "Particles must not have ghosts before distribution.");
    if (comm_.size() == 1) return;
    const auto my_rank = comm_.rank();

    // Snapshot the global particle data. The array is identical on all the
    // processes, and so is the layout: all the fluid particles, ordered by
    // the global identifier, then all the fixed particles.
    GlobalArray_<PA> global{};
    global.positions.assign_range(sph::r[particles]);
    global.gids.assign_range(std::as_const(particles)[gid]);
    global.num_fluid = std::ranges::size(particles.fluid());

    // Partition the particles.
    compute_labels_(global);

    // Keep only the particles of our subdomain.
    std::vector<std::size_t> erased{};
    for (const auto index :
         std::views::iota(std::size_t{0}, global.positions.size())) {
      if (global.labels[index] != my_rank) erased.push_back(index);
    }
    particles.erase_owned(erased);

    // Establish the ghost mirrors.
    build_plan_(global, particles);
  }

  /// Re-decompose the domain, migrate the particle ownership, and rebuild
  /// the ghost particle mirrors.
  ///
  /// Must be invoked between the time steps only: within a step the set of
  /// the local particles must stay stable.
  template<sph::particle_array PA>
  void rebuild(PA& particles) {
    TIT_PROFILE_SECTION("dist::ParticleExchange::rebuild()");
    if (comm_.size() == 1) return;
    const auto my_rank = comm_.rank();

    // The ghosts of the previous step are void.
    particles.clear_ghosts();

    // Gather the global particle data: positions and identifiers of the
    // owned particles of every process, in the canonical order.
    auto global = gather_global_(particles);
    const auto my_offset = global.offsets[my_rank];

    // Partition the particles.
    compute_labels_(global);

    // Collect the emigrating particles per destination process.
    const auto num_ranks = comm_.size();
    std::vector<std::vector<std::size_t>> move_out(num_ranks);
    std::vector<std::size_t> erased{};
    for (const auto local :
         std::views::iota(std::size_t{0}, particles.num_owned())) {
      const auto label = global.labels[my_offset + local];
      if (label == my_rank) continue;
      move_out[label].push_back(local);
      erased.push_back(local);
    }

    // Exchange the emigrating particles: pack the full varying state,
    // partitioned by type within each message.
    constexpr auto fields = PA::varying_fields;
    constexpr auto width = PA::pack_width(fields);
    const auto is_fluid = [&particles](std::size_t local) {
      return particles.has_type(local, sph::ParticleType::fluid);
    };
    std::vector<std::size_t> send_fluid_counts(num_ranks);
    std::vector<std::size_t> send_counts(num_ranks);
    send_buffer_.clear();
    for (const auto rank : std::views::iota(std::size_t{0}, num_ranks)) {
      const auto& indices = move_out[rank];
      send_fluid_counts[rank] =
          static_cast<std::size_t>(std::ranges::count_if(indices, is_fluid));
      send_counts[rank] = indices.size() * width;
      const auto offset = send_buffer_.size();
      send_buffer_.resize(offset + send_counts[rank]);
      particles.pack(indices, fields, std::span{send_buffer_}.subspan(offset));
    }
    const auto recv_fluid_counts = comm_.all_to_all(send_fluid_counts);
    const auto recv_counts = comm_.all_to_all(send_counts);
    recv_buffer_.resize(
        std::ranges::fold_left(recv_counts, std::size_t{0}, std::plus{}));
    comm_.all_to_all_v(send_buffer_, send_counts, recv_buffer_, recv_counts);

    // Erase the emigrated particles and append the immigrated ones.
    particles.erase_owned(erased);
    std::size_t total_fluid = 0;
    std::size_t total_fixed = 0;
    for (const auto rank : std::views::iota(std::size_t{0}, num_ranks)) {
      total_fluid += recv_fluid_counts[rank];
      total_fixed += recv_counts[rank] / width - recv_fluid_counts[rank];
    }
    std::size_t fluid_base = 0;
    if (total_fluid > 0) {
      auto appended = particles.append_n(sph::ParticleType::fluid, total_fluid);
      fluid_base = (*appended.begin()).index();
    }
    std::size_t fixed_base = 0;
    if (total_fixed > 0) {
      auto appended = particles.append_n(sph::ParticleType::fixed, total_fixed);
      fixed_base = (*appended.begin()).index();
    }
    std::size_t chunk_offset = 0;
    std::size_t fluid_offset = 0;
    std::size_t fixed_offset = 0;
    for (const auto rank : std::views::iota(std::size_t{0}, num_ranks)) {
      const auto num_particles = recv_counts[rank] / width;
      const auto num_fluid = recv_fluid_counts[rank];
      const auto num_fixed = num_particles - num_fluid;
      const auto chunk =
          std::span{std::as_const(recv_buffer_)}.subspan(chunk_offset,
                                                         recv_counts[rank]);
      particles.unpack(fluid_base + fluid_offset,
                       num_fluid,
                       fields,
                       chunk.first(num_fluid * width));
      particles.unpack(fixed_base + fixed_offset,
                       num_fixed,
                       fields,
                       chunk.subspan(num_fluid * width));
      chunk_offset += recv_counts[rank];
      fluid_offset += num_fluid;
      fixed_offset += num_fixed;
    }

    // Restore the canonical order of the owned particles.
    sort_owned_(particles);

    // Establish the ghost mirrors.
    build_plan_(global, particles);
  }

  /// Refresh the values of the specified fields on the ghost particles.
  template<sph::particle_array PA, field_set Fields>
  void refresh(PA& particles, Fields fields) {
    TIT_PROFILE_SECTION("dist::ParticleExchange::refresh()");
    if (comm_.size() == 1) return;
    const auto num_ranks = comm_.size();
    constexpr auto width = PA::pack_width(Fields{});

    // Pack the mirrored fields of our owned particles per destination.
    std::vector<std::size_t> send_counts(num_ranks);
    std::vector<std::size_t> recv_counts(num_ranks);
    std::size_t send_size = 0;
    std::size_t recv_size = 0;
    for (const auto rank : std::views::iota(std::size_t{0}, num_ranks)) {
      send_counts[rank] = send_indices_[rank].size() * width;
      recv_counts[rank] =
          (recv_counts_[rank][0] + recv_counts_[rank][1]) * width;
      send_size += send_counts[rank];
      recv_size += recv_counts[rank];
    }
    send_buffer_.resize(send_size);
    recv_buffer_.resize(recv_size);
    std::size_t offset = 0;
    for (const auto rank : std::views::iota(std::size_t{0}, num_ranks)) {
      particles.pack(send_indices_[rank],
                     fields,
                     std::span{send_buffer_}.subspan(offset));
      offset += send_counts[rank];
    }

    // Exchange and unpack into the ghost segments.
    comm_.all_to_all_v(send_buffer_, send_counts, recv_buffer_, recv_counts);
    offset = 0;
    for (const auto rank : std::views::iota(std::size_t{0}, num_ranks)) {
      const auto chunk =
          std::span{std::as_const(recv_buffer_)}.subspan(offset,
                                                         recv_counts[rank]);
      const auto [num_fluid, num_fixed] = recv_counts_[rank];
      particles.unpack(recv_offsets_[rank][0],
                       num_fluid,
                       fields,
                       chunk.first(num_fluid * width));
      particles.unpack(recv_offsets_[rank][1],
                       num_fixed,
                       fields,
                       chunk.subspan(num_fluid * width));
      offset += recv_counts[rank];
    }
  }

  /// Reduce a scalar over all the processes.
  template<class Num>
  auto all_reduce_min(Num value) const -> Num {
    return comm_.all_reduce(value, mpi::Op::min);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  // Snapshot of the global particle data, identical on every process.
  template<sph::particle_array PA>
  struct GlobalArray_ final {
    std::vector<sph::particle_vec_t<PA>> positions; // Rank-major order.
    std::vector<std::uint64_t> gids;                // Ditto.
    std::vector<std::size_t> labels;       // Filled by `compute_labels_`.
    std::vector<std::size_t> offsets;      // Start index of each rank's slice.
    std::vector<std::size_t> fluid_counts; // Owned fluid count per rank.
    std::size_t num_fluid = 0;             // For the replicated snapshot.
    bool replicated = true;
  };

  // Check if the given global index refers to a fluid particle.
  template<sph::particle_array PA>
  static constexpr auto is_fluid_(const GlobalArray_<PA>& global,
                                  std::size_t index) noexcept -> bool {
    if (global.replicated) return index < global.num_fluid;
    const auto rank = static_cast<std::size_t>(
        std::ranges::upper_bound(global.offsets, index) -
        global.offsets.begin() - 1);
    return index - global.offsets[rank] < global.fluid_counts[rank];
  }

  // Gather the global particle data from the owned particles.
  template<sph::particle_array PA>
  auto gather_global_(const PA& particles) -> GlobalArray_<PA> {
    using Vec_ = sph::particle_vec_t<PA>;
    GlobalArray_<PA> global{};
    global.replicated = false;

    // Gather the per-rank counts.
    global.fluid_counts =
        comm_.all_gather(std::ranges::size(particles.fluid()));
    const auto owned_counts = comm_.all_gather(particles.num_owned());
    global.offsets.reserve(owned_counts.size());
    std::size_t offset = 0;
    for (const auto num_owned : owned_counts) {
      global.offsets.push_back(offset);
      offset += num_owned;
    }

    // Gather the positions and the identifiers.
    struct Row {
      Vec_ position;
      std::uint64_t gid;
    };
    std::vector<Row> rows{};
    rows.reserve(particles.num_owned());
    for (const auto a : particles.owned()) {
      rows.push_back({sph::r[a], gid[a]});
    }
    const auto [bytes, _] = comm_.all_gather_v(std::as_bytes(std::span{rows}));
    global.positions.reserve(offset);
    global.gids.reserve(offset);
    for (const auto index : std::views::iota(std::size_t{0}, offset)) {
      const auto row = from_bytes<Row>(
          std::span{bytes}.subspan(index * sizeof(Row), sizeof(Row)));
      global.positions.push_back(row.position);
      global.gids.push_back(row.gid);
    }
    return global;
  }

  // Partition the global particle set. Deterministic: every process
  // arrives at the same labels.
  template<sph::particle_array PA>
  void compute_labels_(GlobalArray_<PA>& global) {
    TIT_PROFILE_SECTION("dist::ParticleExchange::compute_labels_()");
    global.labels.resize(global.positions.size());
    partition_func_(global.positions, global.labels, comm_.size());
  }

  // Build the ghost communication plan from the global particle data, and
  // establish the ghost mirrors.
  //
  // The owned particles of the current process must already be in the
  // canonical order matching `global` and `global.labels`.
  template<sph::particle_array PA>
  void build_plan_(const GlobalArray_<PA>& global, PA& particles) {
    TIT_PROFILE_SECTION("dist::ParticleExchange::build_plan_()");
    const auto my_rank = comm_.rank();
    const auto num_ranks = comm_.size();

    // Collect our own globals in the canonical order to be able to convert
    // the global indices into the local ones.
    std::vector<std::size_t> my_globals{};
    for (const auto index :
         std::views::iota(std::size_t{0}, global.positions.size())) {
      if (global.labels[index] == my_rank) my_globals.push_back(index);
    }
    sort_canonically_(global, my_globals);
    TIT_ASSERT(my_globals.size() == particles.num_owned(),
               "Global and local particle counts diverged.");
    const auto local_of =
        [&my_globals, &global, this](std::size_t global_index) -> std::size_t {
      const auto iter = std::ranges::lower_bound(
          my_globals,
          global_index,
          [&global, this](std::size_t i, std::size_t j) {
            return canonical_less_(global, i, j);
          });
      TIT_ASSERT(iter != my_globals.end() && *iter == global_index,
                 "Global particle index is not owned by this process.");
      return static_cast<std::size_t>(iter - my_globals.begin());
    };

    // Find the ghost relations: for each of our particles, every foreign
    // particle within the halo radius is our ghost, and, symmetrically, our
    // particle is a ghost of that particle's owner.
    const auto search_index =
        geom::GridSearch{halo_radius_}(std::views::all(global.positions));
    std::vector<std::vector<std::size_t>> ghost_globals(num_ranks);
    std::vector<std::vector<std::size_t>> send_globals(num_ranks);
    std::vector<std::size_t> hits{};
    for (const auto mine : my_globals) {
      hits.clear();
      search_index.search(geom::BSphere{global.positions[mine], halo_radius_},
                          std::back_inserter(hits));
      for (const auto other : hits) {
        const auto label = global.labels[other];
        if (label == my_rank) continue;
        ghost_globals[label].push_back(other);
        send_globals[label].push_back(mine);
      }
    }

    // Canonicalize the relations. Both sides of every exchange derive the
    // same canonical order independently, so the message layouts agree.
    send_indices_.assign(num_ranks, {});
    recv_offsets_.assign(num_ranks, {});
    recv_counts_.assign(num_ranks, {});
    for (const auto rank : std::views::iota(std::size_t{0}, num_ranks)) {
      auto& ghosts = ghost_globals[rank];
      auto& sends = send_globals[rank];
      sort_canonically_(global, ghosts);
      sort_canonically_(global, sends);
      const auto [ghost_first, ghost_last] = std::ranges::unique(ghosts);
      ghosts.erase(ghost_first, ghost_last);
      const auto [send_first, send_last] = std::ranges::unique(sends);
      sends.erase(send_first, send_last);
      recv_counts_[rank][0] = static_cast<std::size_t>(
          std::ranges::count_if(ghosts, [&global](std::size_t index) {
            return is_fluid_(global, index);
          }));
      recv_counts_[rank][1] = ghosts.size() - recv_counts_[rank][0];
      send_indices_[rank].clear();
      send_indices_[rank].reserve(sends.size());
      for (const auto index : sends) {
        send_indices_[rank].push_back(local_of(index));
      }
    }

    // Append the ghost segments: all the fluid ghosts (per source process,
    // in the process order), then all the fixed ghosts.
    for (const auto type_index :
         std::views::iota(std::size_t{0}, std::size_t{2})) {
      const auto type =
          type_index == 0 ? sph::ParticleType::fluid : sph::ParticleType::fixed;
      std::size_t total = 0;
      for (const auto rank : std::views::iota(std::size_t{0}, num_ranks)) {
        total += recv_counts_[rank][type_index];
      }
      auto ghost_views = particles.append_ghosts_n(type, total);
      auto offset = total > 0 ? (*std::ranges::begin(ghost_views)).index() :
                                std::size_t{0};
      for (const auto rank : std::views::iota(std::size_t{0}, num_ranks)) {
        recv_offsets_[rank][type_index] = offset;
        offset += recv_counts_[rank][type_index];
      }
    }

    // Fill the ghost fields.
    refresh(particles, PA::varying_fields);
  }

  // Canonical global ordering: by type first (fluid before fixed), then by
  // the global identifier.
  template<sph::particle_array PA>
  constexpr auto canonical_less_(const GlobalArray_<PA>& global,
                                 std::size_t i,
                                 std::size_t j) const noexcept -> bool {
    const auto key = [&global](std::size_t index) {
      return std::pair{!is_fluid_(global, index), global.gids[index]};
    };
    return key(i) < key(j);
  }

  template<sph::particle_array PA>
  constexpr void sort_canonically_(const GlobalArray_<PA>& global,
                                   std::vector<std::size_t>& indices) const {
    std::ranges::sort(indices, [&global, this](std::size_t i, std::size_t j) {
      return canonical_less_(global, i, j);
    });
  }

  // Restore the canonical order of the owned particles: each type segment
  // is sorted by the global identifier.
  template<sph::particle_array PA>
  static void sort_owned_(PA& particles) {
    const auto gids = std::as_const(particles)[gid];
    std::vector<std::size_t> perm(particles.num_owned());
    std::ranges::iota(perm, std::size_t{0});
    const auto sort_segment = [&perm, gids](auto segment) {
      if (std::ranges::empty(segment)) return;
      const auto first = (*std::ranges::begin(segment)).index();
      const auto count = std::ranges::size(segment);
      std::ranges::sort(std::span{perm}.subspan(first, count),
                        {},
                        [gids](std::size_t index) { return gids[index]; });
    };
    sort_segment(particles.fluid());
    sort_segment(particles.fixed());
    particles.reorder_owned(perm);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  double halo_radius_;
  [[no_unique_address]] PartitionFunc partition_func_;
  mpi::Comm comm_;

  // Ghost communication plan.
  std::vector<std::vector<std::size_t>> send_indices_;
  std::vector<std::array<std::size_t, 2>> recv_offsets_;
  std::vector<std::array<std::size_t, 2>> recv_counts_;

  // Scratch buffers.
  std::vector<std::byte> send_buffer_;
  std::vector<std::byte> recv_buffer_;

}; // class ParticleExchange

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::dist
