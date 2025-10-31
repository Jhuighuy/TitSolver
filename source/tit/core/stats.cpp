/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <ranges>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/containers/str_hash_map.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/print.hpp"
#include "tit/core/stats.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool Stats::enabled_ = false;
StrHashMap<std::unique_ptr<BaseStatsVar>> Stats::vars_;

void Stats::enable() {
  // Report at exit.
  enabled_ = true;
  const auto status = std::atexit(&report_);
  TIT_ENSURE(status == 0, "Unable to register at-exit callback!");
}

void Stats::report_() {
  // Gather the variables and sort them by name.
  auto sorted_vars = vars_ |
                     std::views::transform([](auto& var) { return &var; }) |
                     std::ranges::to<std::vector>();
  std::ranges::sort(sorted_vars,
                    /*cmp=*/{},
                    [](const auto* var) -> const auto& { return var->first; });

  // Print the report table.
  constexpr size_t name_width = 19;
  constexpr const auto* name_title = "name";
  constexpr const auto* values_title = "value";
  println();
  println("Statistics report:");
  println();
  println_separator();
  println("{:<{}} {}", name_title, name_width, values_title);
  println_separator();
  for (const auto* named_var : sorted_vars) {
    const auto& [name, var_ptr] = *named_var;
    println("{:<{}} min: {}", "  ", name_width, var_ptr->render_min());
    println("{:<{}} avg: {}", name, name_width, var_ptr->render_avg());
    println("{:<{}} max: {}", "  ", name_width, var_ptr->render_max());
    println_separator();
  }
  println();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
