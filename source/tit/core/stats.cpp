/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cstdio>
#include <memory>
#include <mutex>
#include <ranges>
#include <vector>

#include "tit/core/io.hpp"
#include "tit/core/stats.hpp"
#include "tit/core/string_utils.hpp"
#include "tit/core/sys.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

std::mutex Stats::vars_mutex_;
StringHashMap<std::unique_ptr<BaseStatsVar>> Stats::vars_;

void Stats::enable() noexcept {
  // Report at exit.
  safe_atexit(&report_);
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
  const auto width = tty_width(stdout);
  println();
  println("Statistics report:");
  println();
  println("{:->{}}", "", width);
  println();
  for (const auto* named_var : sorted_vars) {
    const auto& [name, var_ptr] = *named_var;
    println("{}", name);
    println("{:~>{}}", "", name.size());
    println("   min. value: {}", var_ptr->render_min());
    println("   avg. value: {}", var_ptr->render_avg());
    println("   max. value: {}", var_ptr->render_max());
    println();
  }
  println("{:->{}}", "", width);
  println();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
