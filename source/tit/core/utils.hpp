/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <utility>

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wrap a macro argument with commas to pass it to another macro.
#define TIT_PASS(...) __VA_ARGS__

/// Concatenate macro arguments.
#define TIT_CAT(a, b) TIT_CAT_IMPL(a, b)
#define TIT_CAT_IMPL(a, b) a##b

/// Generate a unique identifier
#define TIT_NAME(prefix) TIT_CAT(TIT_CAT(prefix, _), __LINE__)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Use this function to assume forwarding references as universal references
/// to avoid false alarms from analysis tools.
#define TIT_ASSUME_UNIVERSAL(T, ref) static_cast<void>(std::forward<T>(ref))

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Default-initialize a type.
template<std::default_initializable T>
constexpr auto zero(const T& /*a*/) -> T {
  return T{};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Predicate that is always true.
struct AlwaysTrue final {
  constexpr auto operator()(const auto& /*arg*/) const noexcept -> bool {
    return true;
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Converter to a different type.
template<class Val>
struct To final {
  template<class... Args>
  static constexpr auto operator()(Args&&... args) noexcept -> Val {
    return Val(std::forward<Args>(args)...);
  }
};

/// Convert to a different type.
template<class Val>
inline constexpr To<Val> to{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Update a value, restoring the old value on destruction.
template<class Val>
class ScopedVal final {
public:

  /// Capture an existing value and replace it with a new one.
  constexpr ScopedVal(Val& ref, Val new_value) noexcept
      : ptr_{std::addressof(ref)},
        old_{std::exchange(*ptr_, std::move(new_value))} {}

  /// Move-construct a scoped value.
  constexpr ScopedVal(ScopedVal&& other) noexcept
      : ptr_{std::exchange(other.ptr_, nullptr)}, old_{std::move(other.old_)} {}

  /// Move-assign a scoped value.
  constexpr auto operator=(ScopedVal&& other) noexcept -> ScopedVal& {
    if (this != &other) {
      restore();
      ptr_ = std::exchange(other.ptr_, nullptr);
      old_ = std::move(other.old_);
    }
    return *this;
  }

  /// Scoped values are not copyable.
  constexpr ScopedVal(const ScopedVal&) = delete;

  /// Scoped values are not copy-assignable.
  constexpr auto operator=(const ScopedVal&) -> ScopedVal& = delete;

  /// Restore the old value.
  constexpr ~ScopedVal() noexcept {
    restore();
  }

  /// Restore the old value.
  void restore() noexcept {
    if (ptr_ == nullptr) return;
    *ptr_ = std::move(old_);
    ptr_ = nullptr;
  }

private:

  Val* ptr_;
  Val old_;

}; // class ScopedVal

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
