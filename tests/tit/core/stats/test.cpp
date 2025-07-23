/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <vector>

#include "tit/core/print.hpp"
#include "tit/core/stats.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void stat_scalar() {
  println("stat_scalar");
  TIT_STATS("scalar", -6);
  for (int i = -5; i <= 5; ++i) {
    TIT_STATS("scalar", i);
  }
  TIT_STATS("scalar", 6);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void stat_vector() {
  println("stat_vector");
  for (int i = -5; i <= 5; ++i) {
    std::vector v{i, i % 2, i % 3};
    v.resize((i + 5) % 4);
    TIT_STATS("vector", v);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void stat_empty_vector() {
  println("stat_empty_vector");
  TIT_STATS("empty_vector", std::vector<int>{});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

auto main() noexcept(false) -> int {
  tit::Stats::enable();
  tit::stat_scalar();
  tit::stat_vector();
  tit::stat_empty_vector();
  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
