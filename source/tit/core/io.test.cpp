/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/io.hpp"

#include "tit/testing/test.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class TestPipe : public Pipe<int, int> {
public:

  void pipe(Source<int>& source, Sink<int>& sink) override {
    for (const auto& item : source.pull(10)) {
      const auto square = item * item;
      sink.push(std::span{&square, 1});
    }
  }

}; // class TestPipe

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Pipe") {
  std::vector<int> items{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  RangeSource source{std::views::all(items)};
  TestPipe pipe{};
  std::vector<int> result{};
  auto sink = make_sink(result);
  PipedSource<int, int> piped_source{source, pipe};
  CHECK_RANGE_EQ(piped_source.pull(5), std::vector{1, 4, 9, 16, 25});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
