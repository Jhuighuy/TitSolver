/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <type_traits>
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

/// Forward a member of a class.
#define TIT_FORWARD_LIKE(self, member) std::forward_like<decltype(self)>(member)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Predicate that is always true.
struct AlwaysTrue final {
  constexpr auto operator()(const auto& /*arg*/) const noexcept -> bool {
    return true;
  }
};

/// Check if the given value is in the range [ @p a, @p b ].
template<class T>
constexpr auto in_range(T x,
                        std::type_identity_t<T> a,
                        std::type_identity_t<T> b) noexcept -> bool {
  return a <= x && x <= b;
}

/// Check that the value is equal to any of the given values.
template<class T, std::same_as<T>... Us>
constexpr auto is_any_of(T x, Us... us) noexcept -> bool {
  return (... || (x == us));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Shared pointer with custom retain and release functions.
template<class T, std::invocable<T*> RetainFunc, std::invocable<T*> ReleaseFunc>
class SharedPtr final {
public:

  /// Construct a shared pointer.
  constexpr explicit SharedPtr(T* ptr = nullptr,
                               RetainFunc retain = {},
                               ReleaseFunc release = {}) noexcept
      : ptr_{ptr}, retain_{std::move(retain)}, release_{std::move(release)} {}

  /// Move-construct the shared pointer.
  constexpr SharedPtr(SharedPtr&& other) noexcept
      : ptr_{std::exchange(other.ptr_, nullptr)},
        retain_{std::exchange(other.retain_, {})},
        release_{std::exchange(other.release_, {})} {}

  /// Copy-construct the shared pointer.
  constexpr SharedPtr(const SharedPtr& other)
      : ptr_{other.ptr_}, retain_{other.retain_}, release_{other.release_} {
    do_retain_();
  }

  /// Move-assign the shared pointer.
  constexpr auto operator=(SharedPtr&& other) noexcept -> SharedPtr& {
    if (this != &other) {
      do_release_();
      ptr_ = std::exchange(other.ptr_, nullptr);
      retain_ = std::exchange(other.retain_, {});
      release_ = std::exchange(other.release_, {});
    }
    return *this;
  }

  /// Copy-assign the shared pointer.
  constexpr auto operator=(const SharedPtr& other) -> SharedPtr& {
    if (this != &other) {
      do_release_();
      ptr_ = other.ptr_;
      retain_ = other.retain_;
      release_ = other.release_;
      do_retain_();
    }
    return *this;
  }

  /// Destructor.
  constexpr ~SharedPtr() noexcept {
    do_release_();
  }

  /// Get the pointer.
  constexpr auto get() const noexcept -> T* {
    return ptr_;
  }

  /// Reset the pointer.
  constexpr void reset(T* ptr = nullptr) noexcept {
    do_release_();
    ptr_ = ptr;
  }

  /// Release the pointer.
  constexpr auto release() noexcept -> T* {
    return std::exchange(ptr_, nullptr);
  }

  /// Compare shared pointers.
  /// @{
  friend constexpr auto operator==(const SharedPtr& a, const SharedPtr& b)
      -> bool {
    return a.ptr_ == b.ptr_;
  }
  friend constexpr auto operator==(const SharedPtr& a,
                                   const std::nullptr_t /*b*/) -> bool {
    return a.ptr_ == nullptr;
  }
  friend constexpr auto operator==(const std::nullptr_t /*a*/,
                                   const SharedPtr& b) -> bool {
    return b.ptr_ == nullptr;
  }
  /// @}

private:

  constexpr void do_retain_() {
    if (ptr_ != nullptr) retain_(ptr_);
  }

  constexpr void do_release_() //
      noexcept(std::is_nothrow_invocable_v<ReleaseFunc, T*>) {
    if (ptr_ != nullptr) release_(ptr_);
  }

  T* ptr_ = nullptr;
  [[no_unique_address]] RetainFunc retain_;
  [[no_unique_address]] ReleaseFunc release_;

}; // class SharedPtr

/// Same as `SharedPtr`, but type argument is a pointer itself.
template<class P, std::invocable<P> RetainFunc, std::invocable<P> ReleaseFunc>
  requires std::is_pointer_v<P>
using Shared = SharedPtr<std::remove_pointer_t<P>, RetainFunc, ReleaseFunc>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Translator for a given key.
template<class Key, class Val>
class Translator final {
public:

  /// Construct a translator for the given key.
  constexpr explicit Translator(Key key) : key_{std::move(key)} {}

  /// Add an option for the given key and value.
  constexpr auto option(const Key& key, const Val& value) -> Translator& {
    if (!result_ && key == key_) result_ = value;
    return *this;
  }

  /// Fall back to the given value if no value was set, and return it.
  constexpr auto fallback(const Val& value) -> Val {
    return result_ ? std::move(*result_) : value;
  }

  /// Fall back to the given function result if no value was set, and return it.
  template<std::invocable<const Key&> Func>
  constexpr auto fallback(Func func) -> Val {
    if (result_) return std::move(*result_);
    if constexpr (std::same_as<std::invoke_result_t<Func, const Key&>, void>) {
      std::invoke(std::move(func), key_), std::unreachable();
    } else {
      return std::invoke(std::move(func), key_);
    }
  }

  /// Convert the translator to a result.
  constexpr explicit(false) operator Val() {
    return fallback([](const Key& /*key*/) { std::unreachable(); });
  }

private:

  Key key_;
  std::optional<Val> result_;

}; // class Translator

/// Make a translator for the given key.
template<class Val, class Key>
constexpr auto translate(Key key) -> Translator<Key, Val> {
  return Translator<Key, Val>{std::move(key)};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTBEGIN(*-macro-parentheses)

/// Mark the class as move-only.
#define TIT_MOVE_ONLY(Class)                                                   \
  /** This class is not copy-constructible. */                                 \
  Class(const Class&) = delete;                                                \
  /** This class is not copy-assignable. */                                    \
  auto operator=(const Class&)->Class& = delete

/// Mark the class as not copyable or movable.
#define TIT_NOT_COPYABLE_OR_MOVABLE(Class)                                     \
  TIT_MOVE_ONLY(Class);                                                        \
  /** This class is not move-constructible. */                                 \
  Class(Class&&) = delete;                                                     \
  /** This class is not move-assignable. */                                    \
  auto operator=(Class&&)->Class& = delete

// NOLINTEND(*-macro-parentheses)

/// Virtual base class.
class VirtualBase {
public:

  TIT_NOT_COPYABLE_OR_MOVABLE(VirtualBase);

  /// Default constructor.
  constexpr VirtualBase() = default;

  /// Virtual destructor.
  constexpr virtual ~VirtualBase() = default;

}; // class VirtualBase

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
