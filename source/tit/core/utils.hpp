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

/// Update a value, restoring the old value afterwards.
template<class Val>
class ScopedSetting final {
public:

  /// Capture an existing value and update it.
  constexpr ScopedSetting(Val& ref, Val new_value) noexcept
      : ptr_{std::addressof(ref)},
        old_{std::exchange(*ptr_, std::move(new_value))} {}

  /// Move-construct a scoped setting.
  constexpr ScopedSetting(ScopedSetting&& other) noexcept
      : ptr_{std::exchange(other.ptr_, nullptr)}, old_{std::move(other.old_)} {}

  /// Move-assign a scoped setting.
  constexpr auto operator=(ScopedSetting&& other) noexcept -> ScopedSetting& {
    if (this != &other) {
      restore();
      ptr_ = std::exchange(other.ptr_, nullptr);
      old_ = std::move(other.old_);
    }
    return *this;
  }

  /// Scoped settings are not copyable.
  constexpr ScopedSetting(const ScopedSetting&) = delete;

  /// Scoped settings are not copy-assignable.
  constexpr auto operator=(const ScopedSetting&) -> ScopedSetting& = delete;

  /// Restore the old value.
  constexpr ~ScopedSetting() noexcept {
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

}; // class ScopedSetting

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
