/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/vec.hpp"
#include "tit/geom/surface.hpp"
#include "tit/geom/tessellation.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Surface::tessellate") {
  SUBCASE("2D") {
    geom::Surface<Vec<double, 2>> surf;
    surf.append_vert({0.0, 0.0});
    surf.append_vert({3.0, 0.0});
    surf.append_vert({0.0, 4.0});
    surf.append_face({0, 1});
    surf.append_face({1, 2});
    surf.append_face({2, 0});

    const auto tess = geom::tessellate(surf, 1.0);

    CHECK_RANGE_APPROX_EQ(tess.verts(),
                          {
                              {0.0, 0.0},
                              {3.0, 0.0},
                              {0.0, 4.0},
                              {1.0, 0.0},
                              {2.0, 0.0},
                              {2.4, 0.8},
                              {1.8, 1.6},
                              {1.2, 2.4},
                              {0.6, 3.2},
                              {0.0, 3.0},
                              {0.0, 2.0},
                              {0.0, 1.0},
                          });

    CHECK_RANGE_EQ(tess.face_verts(),
                   {
                       {0, 3},
                       {3, 4},
                       {4, 1},
                       {1, 5},
                       {5, 6},
                       {6, 7},
                       {7, 8},
                       {8, 2},
                       {2, 9},
                       {9, 10},
                       {10, 11},
                       {11, 0},
                   });
  }
  SUBCASE("3D") {
    SUBCASE("uniform") {
      geom::Surface<Vec<double, 3>> surf;
      surf.append_vert({0.0, 0.0, 0.0});
      surf.append_vert({2.0, 0.0, 0.0});
      surf.append_vert({0.0, 2.0, 0.0});
      surf.append_vert({0.0, 0.0, 2.0});
      surf.append_face({0, 2, 1});
      surf.append_face({0, 1, 3});
      surf.append_face({0, 3, 2});
      surf.append_face({1, 2, 3});

      const auto tess = geom::tessellate(surf, 2.0);

      CHECK_RANGE_APPROX_EQ(tess.verts(),
                            {
                                {0.0, 0.0, 0.0},
                                {2.0, 0.0, 0.0},
                                {0.0, 2.0, 0.0},
                                {0.0, 0.0, 2.0},
                                {1.0, 1.0, 0.0},
                                {1.0, 0.0, 1.0},
                                {0.0, 1.0, 1.0},
                            });

      CHECK_RANGE_EQ(tess.face_verts(),
                     {
                         {2, 4, 0},
                         {4, 1, 0},
                         {1, 5, 0},
                         {5, 3, 0},
                         {3, 6, 0},
                         {6, 2, 0},
                         {1, 4, 5},
                         {4, 2, 6},
                         {5, 6, 3},
                         {4, 6, 5},
                     });
    }
    SUBCASE("Y longer") {
      geom::Surface<Vec<double, 3>> surf;
      surf.append_vert({0.0, 0.0, 0.0});
      surf.append_vert({2.0, 0.0, 0.0});
      surf.append_vert({0.0, 4.0, 0.0});
      surf.append_vert({0.0, 0.0, 2.0});
      surf.append_face({0, 2, 1});
      surf.append_face({0, 1, 3});
      surf.append_face({0, 3, 2});
      surf.append_face({1, 2, 3});

      const auto tess = geom::tessellate(surf, 2.0);

      CHECK_RANGE_APPROX_EQ(tess.verts(),
                            {
                                {0.0, 0.0, 0.0},
                                {2.0, 0.0, 0.0},
                                {0.0, 4.0, 0.0},
                                {0.0, 0.0, 2.0},
                                {0.0, 2.0, 0.0},
                                {1.0, 2.0, 0.0},
                                {1.0, 0.0, 1.0},
                                {0.0, 2.0, 1.0},
                                {1.0, 1.0, 0.0},
                                {1.5, 1.0, 0.0},
                                {0.5, 3.0, 0.0},
                                {0.0, 1.0, 1.5},
                                {0.0, 1.0, 0.5},
                                {0.0, 3.0, 0.5},
                                {1.0, 1.0, 0.5},
                                {0.5, 1.0, 1.0},
                            });

      CHECK_RANGE_EQ(tess.face_verts(),
                     {
                         {4, 8, 0},   {8, 1, 0},   {5, 9, 4},   {9, 8, 4},
                         {9, 1, 8},   {2, 10, 4},  {10, 5, 4},  {1, 6, 0},
                         {6, 3, 0},   {3, 11, 0},  {11, 12, 0}, {11, 7, 12},
                         {0, 12, 4},  {12, 7, 4},  {7, 13, 4},  {13, 2, 4},
                         {1, 9, 6},   {9, 14, 6},  {9, 5, 14},  {5, 10, 7},
                         {10, 13, 7}, {10, 2, 13}, {6, 15, 3},  {15, 11, 3},
                         {15, 7, 11}, {7, 15, 5},  {15, 14, 5}, {15, 6, 14},
                     });
    }
  }
  SUBCASE("Z longer") {
    geom::Surface<Vec<double, 3>> surf;
    surf.append_vert({0.0, 0.0, 0.0});
    surf.append_vert({2.0, 0.0, 0.0});
    surf.append_vert({0.0, 2.0, 0.0});
    surf.append_vert({0.0, 0.0, 4.0});
    surf.append_face({0, 2, 1});
    surf.append_face({0, 1, 3});
    surf.append_face({0, 3, 2});
    surf.append_face({1, 2, 3});

    const auto tess = geom::tessellate(surf, 2.0);

    CHECK_RANGE_APPROX_EQ(tess.verts(),
                          {
                              {0.0, 0.0, 0.0},
                              {2.0, 0.0, 0.0},
                              {0.0, 2.0, 0.0},
                              {0.0, 0.0, 4.0},
                              {1.0, 1.0, 0.0},
                              {1.0, 0.0, 2.0},
                              {0.0, 0.0, 2.0},
                              {0.0, 1.0, 2.0},
                              {1.5, 0.0, 1.0},
                              {0.5, 0.0, 1.0},
                              {0.5, 0.0, 3.0},
                              {0.0, 1.0, 1.0},
                              {0.0, 1.5, 1.0},
                              {0.0, 0.5, 3.0},
                              {1.0, 0.5, 1.0},
                              {0.5, 1.0, 1.0},
                          });

    CHECK_RANGE_EQ(tess.face_verts(),
                   {
                       {2, 4, 0},   {4, 1, 0},   {1, 8, 0},   {8, 9, 0},
                       {8, 5, 9},   {0, 9, 6},   {9, 5, 6},   {5, 10, 6},
                       {10, 3, 6},  {6, 11, 0},  {11, 2, 0},  {7, 12, 6},
                       {12, 11, 6}, {12, 2, 11}, {3, 13, 6},  {13, 7, 6},
                       {4, 14, 1},  {14, 8, 1},  {14, 5, 8},  {2, 12, 4},
                       {12, 15, 4}, {12, 7, 15}, {7, 13, 5},  {13, 10, 5},
                       {13, 3, 10}, {5, 14, 7},  {14, 15, 7}, {14, 4, 15},
                   });
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
