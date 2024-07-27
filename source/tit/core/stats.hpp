/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <format>
#include <memory>
#include <mutex>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/string_utils.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Base class of a statistics variable.
class BaseStatsVar {
public:

  /// Construct the statistics variable.
  BaseStatsVar() = default;

  /// Statistics variable is not move-constructible.
  BaseStatsVar(BaseStatsVar&&) = delete;

  /// Statistics variable is not copy-constructible.
  BaseStatsVar(const BaseStatsVar&) = delete;

  /// Statistics variable is not movable.
  auto operator=(BaseStatsVar&&) -> BaseStatsVar& = delete;

  /// Statistics variable is not copyable.
  auto operator=(const BaseStatsVar&) -> BaseStatsVar& = delete;

  /// Destroy the statistics variable.
  virtual ~BaseStatsVar() = default;

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
concept stattable = //
    std::is_object_v<Val> && std::semiregular<Val> &&
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
    using std::max;
    using std::min;
    if (count_ == 0) sum_ = min_ = max_ = val;
    else sum_ += val, min_ = min(min_, val), max_ = max(max_, val);
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

  /// Construct the statistics variable.
  StatsVar() = default;

  /// Get the mean value as a string.
  constexpr auto render_avg() const -> std::string override {
    return format_range(sum_ |
                        std::views::transform(
                            [n = count_](const Val& val) { return val / n; }));
  }

  /// Get the minimum value as a string.
  constexpr auto render_min() const -> std::string override {
    return format_range(min_);
  }

  /// Get the maximum value as a string.
  constexpr auto render_max() const -> std::string override {
    return format_range(max_);
  }

  /// Update the statistics variable.
  void update(const Vals& range) {
    using std::max;
    using std::min;
    for (const auto& [i, val] :
         range | std::views::take(sum_.size()) | std::views::enumerate) {
      sum_[i] += val;
      min_[i] = min(min_[i], val);
      max_[i] = max(max_[i], val);
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
    const std::scoped_lock lock{vars_mutex_};
    /// @todo In C++26 there would be no need for `std::string{...}`.
    auto& var = vars_[std::string{var_name}];
    if (!var) var = std::make_unique<StatsVar<Type>>();
    auto* const typed_var = dynamic_cast<StatsVar<Type>*>(var.get());
    TIT_ASSERT(typed_var != nullptr, "Invalid variable type!");
    return *typed_var;
  }

  /// Enable statistics. Report will be printed at exit.
  static void enable() noexcept;

private:

  static void report_();

  static std::mutex vars_mutex_;
  static StringHashMap<std::unique_ptr<BaseStatsVar>> vars_;

}; // class Stats

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Update the statistics variable.
#define TIT_STATS(var_name, ...)                                               \
  TIT_SAVED_VALUE(                                                             \
      tit::Stats::var<std::remove_cvref_t<decltype(__VA_ARGS__)>>(var_name))   \
      .update(__VA_ARGS__)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
