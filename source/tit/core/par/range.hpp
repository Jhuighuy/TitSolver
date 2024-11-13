/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/par.hpp"
#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/missing.hpp" // IWYU pragma: keep
#include "tit/core/par/partitioner.hpp"
#include "tit/core/type_traits.hpp"

namespace tit::par {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Is the range parallelizable via "unviewing"?
template<std::ranges::range Range>
struct is_parallelizable_range : std::bool_constant<blockable_range<Range>> {};

// Specialize the various `std::ranges::view`s.
// Please follow the order as in https://en.cppreference.com/w/cpp/ranges#Views
template<class Base, class Pred>
struct is_parallelizable_range<std::ranges::filter_view<Base, Pred>> :
    is_parallelizable_range<Base> {};

template<class Base, class Proj>
struct is_parallelizable_range<std::ranges::transform_view<Base, Proj>> :
    is_parallelizable_range<Base> {};

template<class Base, size_t N>
struct is_parallelizable_range<std::ranges::elements_view<Base, N>> :
    is_parallelizable_range<Base> {};

template<class Base>
struct is_parallelizable_range<std::ranges::join_view<Base>> :
    is_parallelizable_range<Base> {};

} // namespace impl

/// Parallelizable range.
template<class Range>
concept parallelizable_range =
    impl::is_parallelizable_range<std::remove_cvref_t<Range>>::value;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Get the reference to a function of the `std::ranges::transform_view`.
// This is a hack, because the `std::ranges::transform_view` does
// not expose the function directly.
template<std::ranges::view Base, class Func>
auto HACK_get_transform_view_func(
    std::ranges::transform_view<Base, Func>& view) noexcept -> Func& {
  struct HACK_transform_view {
#ifdef __GLIBCXX__
    Base base;
    [[no_unique_address]] std::ranges::__detail::__box<Func> boxed_func;
#elifdef _LIBCPP_VERSION
    [[no_unique_address]] std::ranges::__movable_box<Func> boxed_func;
    [[no_unique_address]] Base base;
#else
#error Unknown C++ standard library!
#endif
  };
  auto& HACK_view = *std::bit_cast<HACK_transform_view*>(&view);
  return *HACK_view.boxed_func;
}

// Unroll the various `std::ranges::view`s over sized random access ranges, such
// that the the parallel algorithm can be applied to the base sized random
// access range, and the view is applied to the subranges in the loop body.
struct Unview final {
  // Leave the sized random access range as is.
  /// @todo Clang-tidy segfaults if this is a `static` function.
  template<blockable_range Range, class Func>
  constexpr auto operator()(Range&& range, Func func) const noexcept {
    return std::pair{std::views::all(std::forward<Range>(range)),
                     std::move(func)};
  }

  // Unwrap the `std::ranges::filter_view`.
  template<parallelizable_range Base, class Pred, class Func>
  constexpr auto operator()(this auto& self,
                            std::ranges::filter_view<Base, Pred> filter_view,
                            Func func) noexcept {
    return self(std::move(filter_view).base(),
                [func = std::move(func), pred = std::move(filter_view).pred()](
                    std::ranges::view auto subrange,
                    auto... body_args) {
                  return std::invoke(
                      func,
                      std::views::filter(std::move(subrange), std::ref(pred)),
                      std::move(body_args)...);
                });
  }

  // Unwrap the `std::ranges::transform_view`.
  template<parallelizable_range Base, class Proj, class Func>
    requires (!blockable_range<std::ranges::transform_view<Base, Proj>>)
  constexpr auto operator()(
      this auto& self,
      std::ranges::transform_view<Base, Proj> transform_view,
      Func func) noexcept {
    return self(
        std::move(transform_view).base(),
        [func = std::move(func),
         proj = std::move(HACK_get_transform_view_func(transform_view))]( //
            std::ranges::view auto subrange,
            auto... body_args) {
          return std::invoke(
              func,
              std::views::transform(std::move(subrange), std::ref(proj)),
              std::move(body_args)...);
        });
  }

  // Unwrap the `std::ranges::join_view`.
  template<parallelizable_range Base, class Func>
  constexpr auto operator()(this auto& self,
                            std::ranges::join_view<Base> join_view,
                            Func func) noexcept {
    return self(std::move(join_view).base(),
                [func = std::move(func)](std::ranges::view auto subrange,
                                         auto... body_args) {
                  return std::invoke(func,
                                     std::views::join(std::move(subrange)),
                                     std::move(body_args)...);
                });
  }

  // Unwrap the `std::ranges::elements_view`.
  template<parallelizable_range Base, size_t N, class Func>
    requires (!blockable_range<std::ranges::elements_view<Base, N>>)
  constexpr auto operator()(this auto& self,
                            std::ranges::elements_view<Base, N> elements_view,
                            Func func) noexcept {
    return self(std::move(elements_view).base(),
                [func = std::move(func)](std::ranges::view auto subrange,
                                         auto... body_args) {
                  return std::invoke(
                      func,
                      std::views::elements<N>(std::move(subrange)),
                      std::move(body_args)...);
                });
  }

}; // struct Unview

inline constexpr Unview unview{};

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Helper base class that provides `operator()` overload when the range is
/// not a basic parallelizable range and requires unviewing.
///
/// @tparam RangeIndex Index of the range in the argument list.
/// @tparam FuncIndex  Index of the function in the argument list.
template<size_t RangeIndex, size_t FuncIndex>
struct UnviewInvoker {
  /// Invoke the algorithm with the auto partitioner.
  template<std::derived_from<UnviewInvoker> Self, class... Args>
    requires ((RangeIndex < sizeof...(Args) && FuncIndex < sizeof...(Args)) &&
              (parallelizable_range<type_at_t<RangeIndex, Args && ...>> &&
               !blockable_range<type_at_t<RangeIndex, Args && ...>>) )
  auto operator()(this Self& self, Args&&... args) {
    // The code below simply packs the arguments into a tuple, and then
    // "applies" (like `std::apply`) the function to the tuple, replacing the
    // range and the function with the unviewed versions.
    auto args_tuple =
        std::forward_as_tuple<Args...>(std::forward<Args>(args)...);
    auto [range, func] =
        impl::unview(std::forward<type_at_t<RangeIndex, Args&&...>>(
                         std::get<RangeIndex>(args_tuple)),
                     std::move(std::get<FuncIndex>(args_tuple)));
    const auto get_arg =
        [&args_tuple, &range, &func]<size_t Index>(
            std::index_sequence<Index> /*index*/) -> decltype(auto) {
      // Return unviewed versions of the range and the function, and
      // perfect-forward the rest of the arguments.
      if constexpr (Index == RangeIndex) return std::move(range);    // NOLINT
      else if constexpr (Index == FuncIndex) return std::move(func); // NOLINT
      else {
        return std::forward<type_at_t<Index, Args&&...>>(
            std::get<Index>(args_tuple));
      }
    };
    return [&]<size_t... Indices>(std::index_sequence<Indices...> /*indices*/) {
      return self(get_arg(std::index_sequence<Indices>{})...);
    }(std::make_index_sequence<sizeof...(Args)>{});
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::par
