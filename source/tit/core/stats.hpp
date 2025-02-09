/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <format>
#include <functional>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/missing.hpp" // IWYU pragma: keep
#include "tit/core/str_utils.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Base class of a statistics variable.
class BaseStatsVar : public VirtualBase {
public:

  /// Get the average value as a string.
  constexpr virtual auto render_avg() const -> std::string = 0;

  /// Get the minimum value as a string.
  constexpr virtual auto render_min() const -> std::string = 0;

  /// Get the maximum value as a string.
  constexpr virtual auto render_max() const -> std::string = 0;

}; // class BaseStatsVar

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Valid underlying types for statistics variables.
template<class Val>
concept stattable = std::is_object_v<Val> && std::semiregular<Val> &&
                    std::formattable<Val, char> && requires(Val val, size_t n) {
                      // Can compute the average value.
                      { val += val } -> std::convertible_to<Val&>;
                      { val / n } -> std::convertible_to<Val>;
                      // Can compute the minimum and maximum values.
                      { std::min(val, val) } -> std::convertible_to<Val>;
                      { std::max(val, val) } -> std::convertible_to<Val>;
                    };

/// Statistics variable.
template<class Val>
class StatsVar;

template<stattable Val>
class StatsVar<Val> final : public BaseStatsVar {
public:

  /// Get the mean value as a string.
  constexpr auto render_avg() const -> std::string override {
    return std::format("{}", sum_ / count_);
  }

  /// Get the minimum value as a string.
  constexpr auto render_min() const -> std::string override {
    return std::format("{}", min_);
  }

  /// Get the maximum value as a string.
  constexpr auto render_max() const -> std::string override {
    return std::format("{}", max_);
  }

  /// Update the statistics variable.
  void update(const Val& val) {
    if (count_ == 0) sum_ = min_ = max_ = val;
    else sum_ += val, min_ = std::min(min_, val), max_ = std::max(max_, val);
    count_ += 1;
  }

private:

  size_t count_ = 0;
  Val sum_{};
  Val min_{};
  Val max_{};

}; // class StatsVar

template<std::ranges::input_range Vals>
  requires stattable<std::ranges::range_value_t<Vals>>
class StatsVar<Vals> final : public BaseStatsVar {
public:

  /// Value type.
  using Val = std::ranges::range_value_t<Vals>;

  /// Get the mean value as a string.
  constexpr auto render_avg() const -> std::string override {
    const auto averages =
        sum_ |
        std::views::transform(std::bind_back(std::divides<Val>{}, count_));
    return std::format("{}", averages);
  }

  /// Get the minimum value as a string.
  constexpr auto render_min() const -> std::string override {
    return std::format("{}", min_);
  }

  /// Get the maximum value as a string.
  constexpr auto render_max() const -> std::string override {
    return std::format("{}", max_);
  }

  /// Update the statistics variable.
  void update(const Vals& range) {
    for (const auto& [i, val] :
         std::views::enumerate(range) | std::views::take(sum_.size())) {
      sum_[i] += val;
      min_[i] = std::min(min_[i], val);
      max_[i] = std::max(max_[i], val);
    }
    for (const auto& val : range | std::views::drop(sum_.size())) {
      sum_.push_back(val);
      min_.push_back(val);
      max_.push_back(val);
    }
    count_ += 1;
  }

private:

  size_t count_ = 0;
  std::vector<Val> sum_{};
  std::vector<Val> min_{};
  std::vector<Val> max_{};

}; // class StatsVar

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Statistics interface.
class Stats {
public:

  /// Statistics is a static object.
  Stats() = delete;

  /// Profiling variable.
  template<class Type>
    requires std::is_object_v<Type>
  static auto var(std::string_view var_name) -> StatsVar<Type>& {
    /// @todo In C++26 there would be no need for `std::string{...}`.
    auto& var = vars_[std::string{var_name}];
    if (var == nullptr) var = std::make_unique<StatsVar<Type>>();
    auto* const result = dynamic_cast<StatsVar<Type>*>(var.get());
    TIT_ASSERT(result != nullptr, "Type mismatch!");
    return *result;
  }

  /// Enable statistics. Report will be printed at exit.
  static void enable() noexcept;

  /// Is statistics enabled?
  static auto enabled() noexcept -> bool {
    return enabled_;
  }

private:

  static void report_();

  static bool enabled_;
  static StrHashMap<std::unique_ptr<BaseStatsVar>> vars_;

}; // class Stats

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Update the statistics variable.
#define TIT_STATS(var_name, ...)                                               \
  do {                                                                         \
    if (tit::Stats::enabled()) {                                               \
      using TIT_NAME(Type) = std::remove_cvref_t<decltype(__VA_ARGS__)>;       \
      static auto& TIT_NAME(var) = tit::Stats::var<TIT_NAME(Type)>(var_name);  \
      TIT_NAME(var).update(__VA_ARGS__);                                       \
    }                                                                          \
  } while (false)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
