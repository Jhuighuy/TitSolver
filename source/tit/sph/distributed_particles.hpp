/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <cstddef>
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

  template<particle_array ParticleArray>
  constexpr void migrate(ParticleArray& /*particles*/) const noexcept {}

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
        halo_width_{halo_width} {
    TIT_ENSURE(lower_ < upper_, "Slab decomposition bounds are invalid.");
    TIT_ENSURE(halo_width_ > Num{}, "Halo width must be positive.");
  }

  /// Rank that owns a position. Values outside the domain clamp to edge ranks.
  template<class Position>
  auto owner(const Position& position) const -> std::size_t {
    const auto count = communicator_.size();
    const auto fraction =
        std::clamp((position[0] - lower_) / (upper_ - lower_), Num{}, Num{1});
    const auto scaled = fraction * static_cast<Num>(count);
    return std::min(static_cast<std::size_t>(scaled), count - 1);
  }

  /// Rebuild all process-local ghost records from current owned state.
  template<particle_array<r> ParticleArray>
  auto exchange_halos(ParticleArray& particles) const -> bool {
    particles.clear_ghosts();
    const auto rank = communicator_.rank();
    const auto count = communicator_.size();
    std::vector<std::vector<std::byte>> send_buffers(count);

    for (const auto particle : particles.owned()) {
      const auto x = r[particle][0];
      for (std::size_t destination = 0; destination < count; ++destination) {
        if (destination == rank) continue;
        const auto [slab_lower, slab_upper] = bounds_(destination);
        if (x < slab_lower - halo_width_ || x > slab_upper + halo_width_) {
          continue;
        }
        append_record_(send_buffers[destination], particles, particle.index());
      }
    }

    const auto receive_buffers = communicator_.all_to_all_bytes(send_buffers);
    for (const auto& buffer : receive_buffers) {
      append_records_(buffer, particles, true);
    }
    return true;
  }

  /// Transfer accepted owned state to the rank containing its current position.
  template<particle_array<r> ParticleArray>
  void migrate(ParticleArray& particles) const {
    particles.clear_ghosts();
    const auto rank = communicator_.rank();
    std::vector<std::vector<std::byte>> send_buffers(communicator_.size());
    std::vector<std::size_t> outbound;

    for (std::size_t index = 0; index < particles.num_owned(); ++index) {
      const auto destination = owner(r[particles[index]]);
      if (destination == rank) continue;
      append_record_(send_buffers[destination], particles, index);
      outbound.push_back(index);
    }

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

  auto communicator() const noexcept -> const dist::Communicator& {
    return communicator_;
  }

private:

  auto bounds_(std::size_t rank) const -> std::pair<Num, Num> {
    TIT_ASSERT(rank < communicator_.size(), "Slab rank is out of range.");
    const auto width =
        (upper_ - lower_) / static_cast<Num>(communicator_.size());
    const auto slab_lower = lower_ + static_cast<Num>(rank) * width;
    return {slab_lower, slab_lower + width};
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

  dist::Communicator communicator_;
  Num lower_;
  Num upper_;
  Num halo_width_;

}; // class SlabParticleTopology

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
