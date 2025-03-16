/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/sort/morton_curve_sort.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Vec2D = Vec<double, 2>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::MortonCurveSort") {
  // Create points on a 8x8 lattice.
  std::array<Vec2D, 64> points{};
  for (size_t i = 0; i < 64; ++i) points[i] = {i % 8, i / 8};

  // Sort points using the Morton curve.
  std::array<size_t, 64> perm{};
  geom::morton_curve_sort(points, perm);

  // Ensure the resulting permutation is correct.
  //
  //   |   0    1    2    3    4    5    6    7
  // --+---|----|----|----|----|----|----|----|---->
  //   |                                             X
  // 0 -   00--.01  .02--.03   04--.05  .06--.07
  //   |      /    /    /     /   /    /    /
  // 1 -   08'--09'  10'--11 | 12'--13'  14'--15
  //   |    .-------------'  |  .-------------'
  // 2 -   16--.17  .18--.19 | 20--.21  .22--.23
  //   |      /    /    /   /     /    /    /
  // 3 -   24'--25'  26'--27   28'--29'  30'--31
  //   |    .---------------------------------'
  // 4 -   32--.33  .34--.35   36--.37  .38--.39
  //   |      /    /    /     /   /    /    /
  // 5 -   40'--41'  42'--43 | 44'--45'  46'--47
  //   |    .-------------'  |  .-------------'
  // 6 -   48--.49  .50--.51 | 52--.53  .54--.55
  //   |      /    /    /   /     /    /    /
  // 7 -   56'--57'  58'--59   60'--61'  62'--63
  //   |
  //   v
  //     Y
  //
  CHECK_RANGE_EQ(perm, {0,  1,  8,  9,  2,  3,  10, 11, 16, 17, 24, 25, 18,
                        19, 26, 27, 4,  5,  12, 13, 6,  7,  14, 15, 20, 21,
                        28, 29, 22, 23, 30, 31, 32, 33, 40, 41, 34, 35, 42,
                        43, 48, 49, 56, 57, 50, 51, 58, 59, 36, 37, 44, 45,
                        38, 39, 46, 47, 52, 53, 60, 61, 54, 55, 62, 63});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
