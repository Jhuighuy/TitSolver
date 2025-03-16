/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/sort/hilbert_curve_sort.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Vec2D = Vec<double, 2>;
using Vec3D = Vec<double, 3>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::HilbertCurveSort") {
  SUBCASE("2D") {
    // Create points on a 8x8 lattice.
    std::array<Vec2D, 64> points{};
    for (size_t i = 0; i < 64; ++i) points[i] = {i % 8, i / 8};

    // Sort points using the Hilbert curve.
    std::array<size_t, 64> perm{};
    geom::hilbert_curve_sort(points, perm);

    // Ensure the resulting permutation is correct.
    //
    //   |   0    1    2    3    4    5    6    7
    // --+---|----|----|----|----|----|----|----|--->
    //   |                                            X
    // 0 +   00   01===02===03   04===05===06   07
    //   |   ||   ||        ||   ||        ||   ||
    // 1 +   08===09   10===11   12===13   14===15
    //   |             ||             ||
    // 2 +   16===17   18===19   20===21   22===23
    //   |   ||   ||        ||   ||        ||   ||
    // 3 +   24   25===26===27   28===29===30   31
    //   |   ||                                 ||
    // 4 +   32===33   34===35===36===37   38===39
    //   |        ||   ||             ||   ||
    // 5 +   40===41   42===43   44===45   46===47
    //   |   ||             ||   ||             ||
    // 6 +   48   49===50   51   52   53===54   55
    //   |   ||   ||   ||   ||   ||   ||   ||   ||
    // 7 +   56===57   58===59   60===61   62===63
    //   |
    //   v
    //     Y
    //
    CHECK_RANGE_EQ(perm, {0,  8,  9,  1,  2,  3,  11, 10, 18, 19, 27, 26, 25,
                          17, 16, 24, 32, 33, 41, 40, 48, 56, 57, 49, 50, 58,
                          59, 51, 43, 42, 34, 35, 36, 37, 45, 44, 52, 60, 61,
                          53, 54, 62, 63, 55, 47, 46, 38, 39, 31, 23, 22, 30,
                          29, 28, 20, 21, 13, 12, 4,  5,  6,  14, 15, 7});
  }

  SUBCASE("3D") {
    // Create points on a 4x4x4 lattice.
    std::array<Vec3D, 64> points{};
    for (size_t i = 0; i < 64; ++i) points[i] = {i % 4, (i / 4) % 4, i / 16};

    // Sort points using the Hilbert curve.
    std::array<size_t, 64> perm{};
    geom::hilbert_curve_sort(points, perm);

    // Ensure the resulting permutation is correct.
    //
    // ... I am unable to draw an ASCII art for this one :(
    //
    CHECK_RANGE_EQ(perm, {0,  4,  5,  1,  17, 21, 20, 16, 32, 33, 49, 48, 52,
                          53, 37, 36, 40, 41, 57, 56, 60, 61, 45, 44, 28, 12,
                          8,  24, 25, 9,  13, 29, 30, 14, 10, 26, 27, 11, 15,
                          31, 47, 46, 62, 63, 59, 58, 42, 43, 39, 38, 54, 55,
                          51, 50, 34, 35, 19, 23, 22, 18, 2,  6,  7,  3});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
