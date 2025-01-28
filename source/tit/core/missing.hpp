/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <ranges>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifdef _LIBCPP_VERSION

#include <array>
#include <cstdio>
#include <functional>
#include <thread>
#include <utility>

#include "tit/core/uint_utils.hpp"
#include "tit/core/utils.hpp"

_LIBCPP_BEGIN_NAMESPACE_STD

namespace ranges::views {

template<class Range>
concept good_range =
    std::ranges::viewable_range<Range> && std::ranges::sized_range<Range> &&
    std::ranges::random_access_range<Range>;

inline constexpr struct {
  template<std::ranges::viewable_range Range>
  constexpr auto operator()(Range&& range) const noexcept {
    // Let's do nothing here.
    return std::views::all(std::forward<Range>(range));
  }
} as_const{};

inline constexpr struct {
  template<good_range Range>
  constexpr auto operator()(Range&& range) const noexcept {
    using Ref = std::ranges::range_reference_t<Range>;
    auto view = std::views::all(std::forward<Range>(range));
    return std::views::iota(0UZ, std::ranges::size(range)) |
           std::views::transform([view = std::move(view)](size_t index) {
             return std::pair<size_t, Ref>{index, view[index]};
           });
  }
} enumerate{};

inline constexpr struct {
  template<good_range Range>
  constexpr auto operator()(Range&& range, size_t chunk_size) const noexcept {
    auto view = std::views::all(std::forward<Range>(range));
    const auto num_chunks = tit::divide_up(std::ranges::size(view), chunk_size);
    return std::views::iota(0UZ, num_chunks) |
           std::views::transform(
               [view = std::move(view), chunk_size](size_t chunk_index) {
                 const auto sz = std::ranges::size(view);
                 const auto first = std::min(sz, chunk_index * chunk_size);
                 const auto last = std::min(sz, (chunk_index + 1) * chunk_size);
                 const auto iter = std::ranges::begin(view);
                 return std::ranges::subrange(iter + first, iter + last);
               });
  }
  constexpr auto operator()(size_t chunk_size) const {
    return __range_adaptor_closure_t(std::bind_back(*this, chunk_size));
  }
} chunk{};

template<size_t N>
struct AdjacentTransformAdaptor {
  static_assert(N == 2, "Only two adjacent elements are supported!");
  template<good_range Range, class Func>
  constexpr auto operator()(Range&& range, Func func) const noexcept {
    auto view = std::views::all(std::forward<Range>(range));
    return std::views::iota(0UZ, std::ranges::size(range) - 1) |
           std::views::transform(
               [view = std::move(view), func = std::move(func)](size_t index) {
                 return func(std::ranges::begin(view)[index],
                             std::ranges::begin(view)[index + 1]);
               });
  }
  template<class Func>
  constexpr auto operator()(Func func) const {
    return __range_adaptor_closure_t(std::bind_back(*this, std::move(func)));
  }
};
template<size_t N>
inline constexpr AdjacentTransformAdaptor<N> adjacent_transform{};

inline constexpr struct {
  template<good_range... Ranges>
  constexpr auto operator()(Ranges&&... ranges) const noexcept {
    static constexpr auto Dim = sizeof...(Ranges);
    const auto flat_size = (std::ranges::size(ranges) * ...);
    std::array views{std::views::all(std::forward<Ranges>(ranges))...};
    using Elem = decltype(auto(views[0][0]));
    return std::views::iota(0UZ, flat_size) |
           std::views::transform([views = std::move(views)](size_t flat_index) {
             std::array<Elem, Dim> items{};
             for (ssize_t axis = Dim - 1; axis >= 0; --axis) {
               const auto size = std::ranges::size(views[axis]);
               const auto index = flat_index % size;
               items[axis] = views[axis][index];
               flat_index /= size;
             }
             return items;
           });
  }
} cartesian_product{};

} // namespace ranges::views

struct jthread : std::thread {
  using std::thread::thread;
  TIT_MOVE_ONLY(jthread);
  jthread(jthread&&) noexcept = default;
  auto operator=(jthread&&) noexcept -> jthread& = default;
  ~jthread() noexcept {
    if (joinable()) join();
  }
};

inline void println() {
  std::puts("");
}

template<class... Ts>
struct move_only_function : std::function<Ts...> {
  TIT_MOVE_ONLY(move_only_function);
  using std::function<Ts...>::function;
  ~move_only_function() noexcept = default;
  move_only_function(move_only_function&&) noexcept = default;
  auto operator=(move_only_function&&) noexcept
      -> move_only_function& = default;
};

_LIBCPP_END_NAMESPACE_STD

#endif // ifdef _LIBCPP_VERSION

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~≈

#ifdef __GLIBCXX__

#include <concepts>
#include <format>
#include <string>

template<std::ranges::input_range Range>
  requires (std::formattable<std::ranges::range_value_t<Range>, char> &&
            !std::constructible_from<std::string, Range &&>)
struct std::formatter<Range> {
  static constexpr auto parse(const std::format_parse_context& context) {
    return context.begin();
  }
  static constexpr auto format(const Range& range,
                               std::format_context& context) {
    auto out = context.out();
    if (std::ranges::empty(range)) return std::format_to(out, "[]");
    out = std::format_to(out, "[{}", *std::begin(range));
    for (const auto& elem : range | std::views::drop(1)) {
      out = std::format_to(out, ", {}", elem);
    }
    return std::format_to(out, "]");
  }
};

#endif // ifdef __GLIBCXX__

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
