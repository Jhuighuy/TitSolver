/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/exception.hpp"
#include "tit/dist/communicator.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Single-rank topology policy.
class LocalParticleTopology final {
public:

  template<particle_array ParticleArray>
  constexpr auto exchange_halos(ParticleArray& /*particles*/) const noexcept
      -> bool {
    return false;
  }

  template<particle_array ParticleArray, field_set Fields>
  constexpr auto exchange_halos(ParticleArray& /*particles*/,
                                Fields /*fields*/) const noexcept -> bool {
    return false;
  }

  template<particle_array ParticleArray, field_set Fields>
  constexpr void update_halo_fields(ParticleArray& /*particles*/,
                                    Fields /*fields*/) const noexcept {}

  template<particle_array ParticleArray>
  constexpr void migrate(ParticleArray& /*particles*/) const noexcept {}

  template<particle_array ParticleArray>
  constexpr void rebalance(ParticleArray& /*particles*/) noexcept {}

}; // class LocalParticleTopology

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Static one-dimensional slab decomposition with rebuilt read-only halos.
template<class Num>
class SlabParticleTopology final {
public:

  explicit SlabParticleTopology(dist::Communicator communicator,
                                Num lower,
                                Num upper,
                                Num halo_width)
      : communicator_{std::move(communicator)}, lower_{lower}, upper_{upper},
        halo_width_{halo_width}, cuts_(communicator_.size() + 1) {
    TIT_ENSURE(lower_ < upper_, "Slab decomposition bounds are invalid.");
    TIT_ENSURE(halo_width_ > Num{}, "Halo width must be positive.");
    for (std::size_t rank = 0; rank <= communicator_.size(); ++rank) {
      cuts_[rank] = lower_ + (upper_ - lower_) * static_cast<Num>(rank) /
                                 static_cast<Num>(communicator_.size());
    }
  }

  /// Rank that owns a position. Values outside the domain clamp to edge ranks.
  template<class Position>
  auto owner(const Position& position) const -> std::size_t {
    const auto first_internal = cuts_.begin() + 1;
    const auto last_internal = cuts_.end() - 1;
    return static_cast<std::size_t>(
        std::upper_bound(first_internal, last_internal, position[0]) -
        first_internal);
  }

  /// Rebuild all process-local ghost records from current owned state.
  template<particle_array<r> ParticleArray>
  auto exchange_halos(ParticleArray& particles) const -> bool {
    if (communicator_.size() == 1) {
      TIT_ASSERT(particles.num_ghosts() == 0,
                 "Single-rank topology contains ghost particles.");
      return false;
    }

    particles.clear_ghosts();
    const auto send_buffers =
        make_halo_send_buffers_(particles,
                                [&](auto& buffer, std::size_t index) {
                                  append_record_(buffer, particles, index);
                                });

    const auto receive_buffers = communicator_.all_to_all_bytes(send_buffers);
    for (const auto& buffer : receive_buffers) {
      append_records_(buffer, particles, true);
    }
    return true;
  }

  /// Rebuild ghosts with only the selected source fields.
  template<particle_array<r> ParticleArray, field_set Fields>
    requires (Fields{} <= ParticleArray::varying_fields)
  auto exchange_halos(ParticleArray& particles, Fields fields) const -> bool {
    if (communicator_.size() == 1) {
      TIT_ASSERT(particles.num_ghosts() == 0,
                 "Single-rank topology contains ghost particles.");
      return false;
    }

    particles.clear_ghosts();
    const auto send_buffers = make_halo_send_buffers_(
        particles,
        [&](auto& buffer, std::size_t index) {
          append_field_record_(buffer, particles, index, fields);
        });
    const auto receive_buffers = communicator_.all_to_all_bytes(send_buffers);
    for (const auto& buffer : receive_buffers) {
      append_field_records_(buffer, particles, fields);
    }
    return true;
  }

  /// Update selected fields without invalidating the existing ghost layout.
  template<particle_array<r> ParticleArray, field_set Fields>
    requires (Fields{} <= ParticleArray::varying_fields)
  void update_halo_fields(ParticleArray& particles, Fields fields) const {
    if (communicator_.size() == 1) {
      TIT_ASSERT(particles.num_ghosts() == 0,
                 "Single-rank topology contains ghost particles.");
      return;
    }

    const auto send_buffers = make_halo_send_buffers_(
        particles,
        [&](auto& buffer, std::size_t index) {
          append_field_record_(buffer, particles, index, fields);
        });

    const auto receive_buffers = communicator_.all_to_all_bytes(send_buffers);
    for (const auto& buffer : receive_buffers) {
      update_field_records_(buffer, particles, fields);
    }
  }

  /// Transfer accepted owned state to the rank containing its current position.
  template<particle_array<r> ParticleArray>
  void migrate(ParticleArray& particles) const {
    particles.clear_ghosts();
    if (communicator_.size() == 1) {
      TIT_ASSERT(std::ranges::all_of(particles.owned(),
                                     [&](const auto particle) {
                                       return owner(r[particle]) == 0;
                                     }),
                 "Single-rank topology does not own all particle state.");
      return;
    }

    const auto rank = communicator_.rank();
    std::vector<std::vector<std::byte>> send_buffers(communicator_.size());
    std::vector<std::size_t> outbound;

    for (std::size_t index = 0; index < particles.num_owned(); ++index) {
      const auto destination = owner(r[particles[index]]);
      if (destination == rank) continue;
      append_record_(send_buffers[destination], particles, index);
      outbound.push_back(index);
    }

    const auto global_outbound = communicator_.all_reduce_sum(
        static_cast<std::uint64_t>(outbound.size()));
    if (global_outbound == 0) return;

    const auto receive_buffers = communicator_.all_to_all_bytes(send_buffers);
    for (const auto index : outbound | std::views::reverse) {
      particles.erase_owned(index);
    }
    for (const auto& buffer : receive_buffers) {
      append_records_(buffer, particles, false);
    }

    TIT_ASSERT(std::ranges::all_of(particles.owned(),
                                   [&](const auto particle) {
                                     return owner(r[particle]) == rank;
                                   }),
               "Particle migration left state on the wrong rank.");
  }

  /// Repartition slabs by an approximate global particle-count quantile and
  /// migrate accepted state to the new owners.
  template<particle_array<r> ParticleArray>
  void rebalance(ParticleArray& particles) {
    if (communicator_.size() == 1) return;

    constexpr std::size_t min_bins = 256;
    const auto num_bins = std::max(min_bins, communicator_.size() * 32);
    std::vector<std::uint64_t> local_histogram(num_bins);
    for (const auto particle : particles.owned()) {
      const auto fraction =
          std::clamp((r[particle][0] - lower_) / (upper_ - lower_),
                     Num{},
                     Num{1});
      const auto bin = std::min(
          static_cast<std::size_t>(fraction * static_cast<Num>(num_bins)),
          num_bins - 1);
      ++local_histogram[bin];
    }
    const auto histogram = communicator_.all_reduce_sum(local_histogram);
    const auto total =
        std::accumulate(histogram.begin(), histogram.end(), std::uint64_t{0});
    if (total == 0) return;

    cuts_.front() = lower_;
    cuts_.back() = upper_;
    std::uint64_t cumulative = 0;
    std::size_t bin = 0;
    const auto num_ranks = static_cast<std::uint64_t>(communicator_.size());
    for (std::size_t rank = 1; rank < communicator_.size(); ++rank) {
      const auto rank_value = static_cast<std::uint64_t>(rank);
      const auto target = (total / num_ranks) * rank_value +
                          ((total % num_ranks) * rank_value) / num_ranks;
      while (bin + 1 < num_bins && cumulative + histogram[bin] < target) {
        cumulative += histogram[bin];
        ++bin;
      }
      cuts_[rank] = lower_ + (upper_ - lower_) * static_cast<Num>(bin + 1) /
                                 static_cast<Num>(num_bins);
    }
    migrate(particles);
  }

  auto communicator() const noexcept -> const dist::Communicator& {
    return communicator_;
  }

private:

  template<particle_array<r> ParticleArray, class AppendRecord>
  auto make_halo_send_buffers_(const ParticleArray& particles,
                               AppendRecord append_record) const
      -> std::vector<std::vector<std::byte>> {
    const auto rank = communicator_.rank();
    const auto count = communicator_.size();
    std::vector<std::vector<std::byte>> send_buffers(count);
    for (const auto particle : particles.owned()) {
      const auto x = r[particle][0];
      const auto [first_destination, last_destination] = halo_destinations_(x);
      for (std::size_t destination = first_destination;
           destination <= last_destination;
           ++destination) {
        if (destination == rank) continue;
        const auto [slab_lower, slab_upper] = bounds_(destination);
        if (x < slab_lower - halo_width_ || x > slab_upper + halo_width_) {
          continue;
        }
        append_record(send_buffers[destination], particle.index());
      }
    }
    return send_buffers;
  }

  /// Inclusive rank range whose halo-extended slabs contain a coordinate.
  auto halo_destinations_(Num x) const -> std::pair<std::size_t, std::size_t> {
    const auto count = communicator_.size();
    const auto first_cut =
        std::lower_bound(cuts_.begin() + 1, cuts_.end(), x - halo_width_);
    const auto first =
        std::min(count - 1,
                 static_cast<std::size_t>(first_cut - (cuts_.begin() + 1)));

    const auto after_last_cut =
        std::upper_bound(cuts_.begin(), cuts_.end() - 1, x + halo_width_);
    const auto last = after_last_cut == cuts_.begin() ?
                          std::size_t{0} :
                          std::min(count - 1,
                                   static_cast<std::size_t>(after_last_cut -
                                                            cuts_.begin() - 1));
    return {first, last};
  }

  auto bounds_(std::size_t rank) const -> std::pair<Num, Num> {
    TIT_ASSERT(rank < communicator_.size(), "Slab rank is out of range.");
    return {cuts_[rank], cuts_[rank + 1]};
  }

  template<particle_array ParticleArray>
  static void append_record_(std::vector<std::byte>& buffer,
                             const ParticleArray& particles,
                             std::size_t index) {
    const auto offset = buffer.size();
    buffer.resize(offset + ParticleArray::packed_particle_size);
    particles.pack(
        index,
        std::span{buffer}.subspan(offset, ParticleArray::packed_particle_size));
  }

  template<particle_array ParticleArray>
  static void append_records_(std::span<const std::byte> buffer,
                              ParticleArray& particles,
                              bool ghosts) {
    constexpr auto record_size = ParticleArray::packed_particle_size;
    TIT_ENSURE(buffer.size() % record_size == 0,
               "Received particle buffer has an invalid size.");
    for (std::size_t offset = 0; offset < buffer.size();
         offset += record_size) {
      const auto record = buffer.subspan(offset, record_size);
      if (ghosts) {
        particles.append_ghost_packed(record);
      } else {
        particles.append_packed(ParticleType::fluid, record);
      }
    }
  }

  template<particle_array ParticleArray, field_set Fields>
  static void append_field_record_(std::vector<std::byte>& buffer,
                                   const ParticleArray& particles,
                                   std::size_t index,
                                   Fields fields) {
    const auto record_size = ParticleArray::packed_size(fields);
    const auto offset = buffer.size();
    buffer.resize(offset + record_size);
    particles.pack(index,
                   std::span{buffer}.subspan(offset, record_size),
                   fields);
  }

  template<particle_array ParticleArray, field_set Fields>
  static void update_field_records_(std::span<const std::byte> buffer,
                                    ParticleArray& particles,
                                    Fields fields) {
    const auto record_size = ParticleArray::packed_size(fields);
    TIT_ENSURE(buffer.size() % record_size == 0,
               "Received particle field buffer has an invalid size.");
    for (std::size_t offset = 0; offset < buffer.size();
         offset += record_size) {
      particles.update_ghost_packed(buffer.subspan(offset, record_size),
                                    fields);
    }
  }

  template<particle_array ParticleArray, field_set Fields>
  static void append_field_records_(std::span<const std::byte> buffer,
                                    ParticleArray& particles,
                                    Fields fields) {
    const auto record_size = ParticleArray::packed_size(fields);
    TIT_ENSURE(buffer.size() % record_size == 0,
               "Received particle field buffer has an invalid size.");
    for (std::size_t offset = 0; offset < buffer.size();
         offset += record_size) {
      particles.append_ghost_packed(buffer.subspan(offset, record_size),
                                    fields);
    }
  }

  dist::Communicator communicator_;
  Num lower_;
  Num upper_;
  Num halo_width_;
  std::vector<Num> cuts_;

}; // class SlabParticleTopology

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
