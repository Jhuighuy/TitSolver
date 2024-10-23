/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"

#include "tit/data/darray.hpp"
#include "tit/data/dtype.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Vec3F = Vec<float, 3>;
using Vec3D = Vec<double, 3>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::DataArray::ctor") {
  SUBCASE("with dtype") {
    const data::DataArray darr{data::dtype_of_v<Vec3D>};
    CHECK(darr.dtype() == data::dtype_of_v<Vec3D>);
    CHECK(darr.empty());
  }
  SUBCASE("with size and dtype") {
    const data::DataArray darr(3, data::dtype_of_v<Vec3D>);
    CHECK(darr.dtype() == data::dtype_of_v<Vec3D>);
    CHECK(darr.size() == 3);
    for (const auto& x : darr.all<Vec3D>()) CHECK(x == Vec3D{});
  }
  SUBCASE("with size and value") {
    const Vec3D val{1.0, 2.0, 3.0};
    const data::DataArray darr(3, val);
    CHECK(darr.size() == 3);
    for (const auto& x : darr.all<Vec3D>()) CHECK(x == val);
  }
  SUBCASE("with initializer list") {
    const Vec3D val_1{1.0, 2.0, 3.0};
    const Vec3D val_2{4.0, 5.0, 6.0};
    const Vec3D val_3{7.0, 8.0, 9.0};
    const data::DataArray darr{val_1, val_2, val_3};
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Element access
//

TEST_CASE("data::DataArray::all") {
  const std::array vals{Vec3D{1.0, 2.0, 3.0},
                        Vec3D{4.0, 5.0, 6.0},
                        Vec3D{7.0, 8.0, 9.0}};
  data::DataArray darr{vals[0], vals[1], vals[2]};
  SUBCASE("get all") {
    const auto& const_darr = darr;
    const auto elems = const_darr.all<Vec3D>();
    CHECK(elems.size() == 3);
    for (size_t i = 0; i < 3; ++i) CHECK(elems[i] == vals[i]);
  }
  SUBCASE("set all") {
    for (auto& x : darr.all<Vec3D>()) x = Vec3D{};
    for (const auto& x : darr.all<Vec3D>()) CHECK(x == Vec3D{});
  }
}

TEST_CASE("data::DataArray::at") {
  const Vec3D val_1{1.0, 2.0, 3.0};
  const Vec3D val_2{4.0, 5.0, 6.0};
  const Vec3D val_3{7.0, 8.0, 9.0};
  data::DataArray darr{val_1, val_2, val_3};
  SUBCASE("get at") {
    const auto& const_darr = darr;
    CHECK(const_darr.at<Vec3D>(0) == val_1);
    CHECK(const_darr.at<Vec3D>(1) == val_2);
    CHECK(const_darr.at<Vec3D>(2) == val_3);
  }
  SUBCASE("set at") {
    darr.at<Vec3D>(1) = val_1;
    darr.at<Vec3D>(2) = val_2;
    darr.at<Vec3D>(0) = val_3;
    CHECK(darr.at<Vec3D>(1) == val_1);
    CHECK(darr.at<Vec3D>(2) == val_2);
    CHECK(darr.at<Vec3D>(0) == val_3);
  }
}

TEST_CASE("data::DataArray::front_and_back") {
  const Vec3D val_1{1.0, 2.0, 3.0};
  const Vec3D val_2{4.0, 5.0, 6.0};
  const Vec3D val_3{7.0, 8.0, 9.0};
  data::DataArray darr{val_1, val_2, val_3};
  SUBCASE("get front/back") {
    const auto& const_darr = darr;
    CHECK(const_darr.front<Vec3D>() == val_1);
    CHECK(const_darr.back<Vec3D>() == val_3);
  }
  SUBCASE("set front/back") {
    darr.front<Vec3D>() = val_3;
    darr.back<Vec3D>() = val_1;
    CHECK(darr.front<Vec3D>() == val_3);
    CHECK(darr.back<Vec3D>() == val_1);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Capacity
//

TEST_CASE("data::DataArray::empty_and_size") {
  const Vec3D val{1.0, 2.0, 3.0};
  data::DataArray darr{val, val, val};

  // Ensure the data array is not empty.
  CHECK(darr.size() == 3);
  CHECK_FALSE(darr.empty());

  // Ensure the data array is empty after clearing.
  darr.clear();
  CHECK(darr.empty());
  CHECK(darr.size() == 0);
}

TEST_CASE("data::DataArray::reserve_and_capacity") {
  const Vec3D val{1.0, 2.0, 3.0};
  data::DataArray darr{val, val, val};

  // Ensure capacity is not zero.
  CHECK(darr.size() == 3);
  CHECK(darr.capacity() != 0);

  // Ensure capacity increases.
  darr.reserve(10);
  CHECK(darr.size() == 3);
  CHECK(darr.capacity() == 10);

  // Ensure capacity does not decrease.
  darr.reserve(5);
  CHECK(darr.size() == 3);
  CHECK(darr.capacity() == 10);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Modifiers
//

TEST_CASE("data::DataArray::clear") {
  const Vec3D val{1.0, 2.0, 3.0};
  data::DataArray darr{val, val, val};

  // Ensure `clear` clears the data array.
  const auto capacity = darr.capacity();
  darr.clear();
  CHECK(darr.empty());
  CHECK(darr.capacity() == capacity);

  // Ensure memory is zeroed after resizing.
  darr.resize(4);
  for (const auto& x : darr.all<Vec3D>()) CHECK(x == Vec3D{});
}

/// @todo `emplace` is missing.

TEST_CASE("data::DataArray::emplace_back") {
  const Vec3D val{1.0, 2.0, 3.0};
  data::DataArray darr{val, val, val};

  // Ensure a new element is correctly emplaced.
  const Vec3D new_val{4.0, 5.0, 6.0};
  darr.emplace_back<Vec3D>(new_val[0], new_val[1], new_val[2]);
  CHECK(darr.size() == 4);
  CHECK(all(darr.back<Vec3D>() == new_val));
}

/// @todo `insert` is missing.

/// @todo `insert_range` is missing.

TEST_CASE("data::DataArray::push_back") {
  const Vec3D val{1.0, 2.0, 3.0};
  data::DataArray darr{val, val, val};

  // Ensure a new element is correctly pushed.
  const Vec3D new_val{4.0, 5.0, 6.0};
  darr.push_back(new_val);
  CHECK(darr.size() == 4);
  CHECK(all(darr.back<Vec3D>() == new_val));
}

/// @todo `erase` is missing.

TEST_CASE("data::DataArray::pop_back") {
  const Vec3D val{1.0, 2.0, 3.0};
  data::DataArray darr{val, val, val};

  // Ensure the last element is correctly removed.
  darr.pop_back();
  CHECK(darr.size() == 2);
  for (const auto& x : darr.all<Vec3D>()) CHECK(x == val);
}

TEST_CASE("data::DataArray::resize") {
  const Vec3D val{1.0, 2.0, 3.0};
  data::DataArray darr{val, val, val};

  // Increase size.
  darr.resize(5);
  CHECK(darr.size() == 5);
  for (const auto& x : darr.all<Vec3D>().subspan(0, 3)) CHECK(x == val);
  for (const auto& x : darr.all<Vec3D>().subspan(4, 2)) CHECK(x == Vec3D{});

  // Decrease size.
  darr.resize(2);
  CHECK(darr.size() == 2);
  for (const auto& x : darr.all<Vec3D>()) CHECK(all(x == val));
}

TEST_CASE("data::DataArray::swap") {
  const Vec3F fval{1.0F, 2.0F, 3.0F};
  const Vec3D dval{1.0, 2.0, 3.0};
  data::DataArray darr_1{fval, fval, fval};
  data::DataArray darr_2{dval, dval};
  darr_1.swap(darr_2);
  CHECK(darr_1.size() == 2);
  CHECK(darr_2.size() == 3);
  CHECK(darr_1.dtype() == data::dtype_of_v<Vec3D>);
  CHECK(darr_2.dtype() == data::dtype_of_v<Vec3F>);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
