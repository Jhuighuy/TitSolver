/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/type.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle exchange interface for the distributed runs.
///
/// The solver is written against this interface, and stays unaware of the
/// underlying communication machinery:
///
/// - `rebuild` re-decomposes the domain, migrates the particle ownership,
///   and rebuilds the ghost particle mirrors. It is invoked between the
///   time steps only: within a step the set of local particles is stable.
///
/// - `refresh` updates the values of the specified fields on the ghost
///   particles from their owning processes. It is invoked between the
///   computational phases whenever a phase reads neighbor fields that the
///   preceding phases have modified.
///
/// - `all_reduce_min` reduces a scalar over all the processes.
template<class X, class PA>
concept exchange_for = particle_array<PA> && requires (X& x, PA& particles) {
  x.rebuild(particles);
  x.refresh(particles, TypeSet{rho});
  {
    x.all_reduce_min(particle_num_t<PA>{})
  } -> std::same_as<particle_num_t<PA>>;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No-op particle exchange for the single-process runs.
class SerialExchange final {
public:

  /// Nothing to rebuild: all the particles are owned, there are no ghosts.
  template<particle_array PA>
  static constexpr void rebuild(PA& /*particles*/) noexcept {
    // Nothing to do.
  }

  /// Nothing to refresh: there are no ghost particles.
  template<particle_array PA, field_set Fields>
  static constexpr void refresh(PA& /*particles*/, Fields /*fields*/) noexcept {
    // Nothing to do.
  }

  /// Reduction over a single process is an identity operation.
  template<class Num>
  static constexpr auto all_reduce_min(Num value) noexcept -> Num {
    return value;
  }

}; // class SerialExchange

/// No-op particle exchange instance.
inline constexpr SerialExchange serial_exchange{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
