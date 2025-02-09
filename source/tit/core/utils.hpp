/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// @todo Restore proper banners once Clang is fixed.
#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <iterator>
#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"

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

/// A pair of values of the same type.
template<class T>
using pair_of_t = std::pair<T, T>;

/// Predicate that is always true.
struct AlwaysTrue {
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

/// Array type that is deduced from the given argument.
/// If the argument is not an array, it is wrapped into an array of size 1.
template<class Arg>
using array_from_arg_t = decltype(std::array{std::declval<Arg>()});

/// Size of the array that is constructed by packing the given values
/// or arrays of values into a single array.
template<class... Args>
inline constexpr auto packed_array_size_v =
    std::tuple_size_v<decltype(std::tuple_cat(array_from_arg_t<Args>{}...))>;

/// Check if the given values or arrays of values can be packed into a single
/// array of the given size and type.
template<size_t Size, class R, class... Args>
concept can_pack_array =
    (... &&
     std::constructible_from<R, typename array_from_arg_t<Args>::value_type>) &&
    (((... + std::tuple_size_v<array_from_arg_t<Args>>) == Size) ||
     (((... + std::tuple_size_v<array_from_arg_t<Args>>) < Size) &&
      std::default_initializable<R>) );

/// Pack values and arrays of values into an array of the given size and type.
template<size_t Size, class R, class... Args>
  requires can_pack_array<Size, R, Args...>
constexpr auto make_array(Args&&... args) -> std::array<R, Size> {
  return std::apply(
      []<class... Elems>(Elems&&... elems) {
        return std::array<R, Size>{R(std::forward<Elems>(elems))...};
      },
      std::tuple_cat(std::array{std::forward<Args>(args)}...));
}

/// Concatenate arrays.
template<size_t... Sizes, class T>
  requires std::copy_constructible<T>
constexpr auto array_cat(const std::array<T, Sizes>&... arrays)
    -> std::array<T, (... + Sizes)> {
  std::array<T, (... + Sizes)> result{};
  auto out_iter = result.begin();
  (..., [&out_iter](const auto& array) {
    out_iter = std::ranges::copy(array, out_iter).out;
  }(arrays));
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Convert a value to a byte array.
template<class T>
  requires std::is_trivially_copyable_v<T>
constexpr auto to_byte_array(const T& value) -> std::array<byte_t, sizeof(T)> {
  return std::bit_cast<std::array<byte_t, sizeof(T)>>(value);
}

/// Convert a value to a byte vector.
template<class T>
  requires std::is_trivially_copyable_v<T>
constexpr auto to_bytes(const T& value) -> std::vector<byte_t> {
  const auto bytes_array = to_byte_array(value);
  return {bytes_array.begin(), bytes_array.end()};
}

/// Convert a byte array to a value.
template<class T>
  requires std::is_trivially_copyable_v<T>
constexpr auto from_bytes(std::span<const byte_t> bytes) -> T {
  TIT_ASSERT(bytes.size() >= sizeof(T), "Invalid byte array size!");
  std::array<byte_t, sizeof(T)> bytes_array{};
  std::ranges::copy(bytes, bytes_array.begin());
  return std::bit_cast<T>(bytes_array);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Range of indices.
template<class Indices>
concept index_range = std::ranges::random_access_range<Indices> &&
                      std::integral<std::ranges::range_value_t<Indices>>;

/// Range of indices that can be used as output for various algorithms.
template<class Indices>
concept output_index_range = std::ranges::random_access_range<Indices> &&
                             std::ranges::output_range<Indices, size_t>;

/// Permuted range view.
template<std::ranges::random_access_range Range, index_range Perm>
  requires std::ranges::viewable_range<Range> &&
           std::ranges::viewable_range<Perm>
constexpr auto permuted_view(Range&& range, Perm&& perm) {
  TIT_ASSUME_UNIVERSAL(Range, range);
  return std::ranges::transform_view{
      std::forward<Perm>(perm),
      [range_view = std::views::all(std::forward<Range>(range))](
          size_t index) -> auto&& { return range_view[index]; }};
}

/// Initialize the permutation with the identity permutation.
/// @{
template<std::ranges::sized_range Range>
[[nodiscard]] constexpr auto iota_perm(Range&& range) noexcept {
  TIT_ASSUME_UNIVERSAL(Range, range);
  return std::views::iota(size_t{0}, std::size(range));
}
template<std::ranges::sized_range Range, output_index_range Perm>
constexpr void iota_perm(Range&& range, Perm&& perm) {
  TIT_ASSUME_UNIVERSAL(Range, range);
  TIT_ASSUME_UNIVERSAL(Perm, perm);
  std::ranges::copy(iota_perm(range), std::begin(perm));
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Non-copyable base class.
class NonCopyableBase {
public:

  /// Construct the non-copyable class instance.
  NonCopyableBase() = default;

  /// Destruct the non-copyable class instance.
  ~NonCopyableBase() = default;

  /// Non-copyable class instance is move-constructible.
  NonCopyableBase(NonCopyableBase&&) = default;

  /// Non-copyable class instance is not copy-constructible.
  NonCopyableBase(const NonCopyableBase&) = delete;

  /// Non-copyable class instance is movable.
  auto operator=(NonCopyableBase&&) -> NonCopyableBase& = default;

  /// Non-copyable class instance is not copyable.
  auto operator=(const NonCopyableBase&) -> NonCopyableBase& = delete;

}; // class NonCopyableBase

/// Non-movable base class.
class NonMovableBase : public NonCopyableBase {
public:

  /// Construct the non-movable class instance.
  NonMovableBase() = default;

  /// Destruct the non-movable class instance.
  ~NonMovableBase() = default;

  /// Non-movable class instance is not move-constructible.
  NonMovableBase(NonMovableBase&&) = delete;

  /// Non-movable class instance is not movable.
  auto operator=(NonMovableBase&&) -> NonMovableBase& = delete;

}; // class NonMovableBase

/// Virtual base class.
class VirtualBase : public NonMovableBase {
public:

  /// Virtual destructor.
  virtual ~VirtualBase() = default;

}; // class VirtualBase

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
