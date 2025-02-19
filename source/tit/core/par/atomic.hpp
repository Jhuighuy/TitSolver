/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <thread>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/type_utils.hpp"

namespace tit::par {

// NOLINTBEGIN(*-pro-type-vararg)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Atomic type.
template<class Val>
concept atomic = std::integral<Val> || std::is_pointer_v<Val>;

/// Memory order.
enum class MemOrder : uint8_t {
  relaxed = __ATOMIC_RELAXED, ///< Relaxed memory order.
  acquire = __ATOMIC_ACQUIRE, ///< Acquire memory order.
  release = __ATOMIC_RELEASE, ///< Release memory order.
  acq_rel = __ATOMIC_ACQ_REL, ///< Acquire-release memory order.
  seq_cst = __ATOMIC_SEQ_CST, ///< Sequentially consistent memory order.
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Atomically load the value.
template<MemOrder Order = MemOrder::acquire, atomic Val>
[[gnu::always_inline]]
inline auto load(const Val& val) noexcept -> Val {
  return __atomic_load_n(&val, std::to_underlying(Order));
}

/// Atomically store the value.
template<MemOrder Order = MemOrder::release, atomic Val>
[[gnu::always_inline]]
inline void store(Val& val, Val desired) noexcept {
  __atomic_store_n(&val, desired, std::to_underlying(Order));
}

/// Atomically wait for the value to change.
template<MemOrder Order = MemOrder::acquire, atomic Val>
[[gnu::always_inline]]
inline auto wait(const Val& val, Val old) noexcept -> Val {
  while (true) {
    if (const auto current = load<Order>(val); current != old) return current;
    std::this_thread::yield();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Atomically compare and exchange the value.
template<MemOrder SuccessOrder = MemOrder::relaxed,
         MemOrder FailureOrder = MemOrder::relaxed,
         atomic Val>
[[gnu::always_inline]]
inline auto compare_exchange(Val& val,
                             std::type_identity_t<Val> expected,
                             std::type_identity_t<Val> desired) noexcept
    -> bool {
  return __atomic_compare_exchange_n( //
      &val,
      &expected,
      desired,
      /*weak=*/false,
      std::to_underlying(SuccessOrder),
      std::to_underlying(FailureOrder));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Atomically perform addition and return what was stored before.
template<MemOrder Order = MemOrder::relaxed, atomic Val>
[[gnu::always_inline]]
inline auto fetch_and_add(Val& val, difference_t<Val> delta) noexcept -> Val {
  return __atomic_fetch_add(&val, delta, std::to_underlying(Order));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-pro-type-vararg)

} // namespace tit::par
