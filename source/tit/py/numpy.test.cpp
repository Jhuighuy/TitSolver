/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>

#include "tit/core/basic_types.hpp"
#include "tit/core/containers/mdvector.hpp"

#include "tit/data/type.hpp"

#include "tit/py/capsule.hpp"
#include "tit/py/error.hpp"
#include "tit/py/number.hpp"
#include "tit/py/numpy.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

#include "tit/py/interpreter.testing.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::NDArray") {
  SUBCASE("typing") {
    CHECK(py::NDArray::type().fully_qualified_name() == "numpy.ndarray");
    CHECK(py::NDArray::isinstance(py::NDArray{Mdvector<int, 1>{}}));
  }
  SUBCASE("construction") {
    SUBCASE("from Mdvector") {
      const Mdvector mdvec{std::array{1.0, 2.0, 3.0, 4.0}.begin(), 2, 2};
      const py::NDArray array{mdvec};
      REQUIRE(array.rank() == 2);
      REQUIRE_RANGE_EQ(array.shape(), std::array{2, 2});
      CHECK(array.elem<double>(0, 0) == 1.0);
      CHECK(array.elem<double>(0, 1) == 2.0);
      CHECK(array.elem<double>(1, 0) == 3.0);
      CHECK(array.elem<double>(1, 1) == 4.0);
      CHECK(py::Capsule::isinstance(array.base()));
    }
  }
  SUBCASE("data access") {
    const std::array vals{1, 2, 3, 4, 5, 6, 7, 8};
    const Mdvector mdvec{vals.begin(), 2, 1, 4};
    const py::NDArray array{mdvec};
    SUBCASE("item") {
      CHECK(array.elem<int>(0, 0, 0) == 1);
      CHECK(array.elem<int>(0, 0, 2) == 3);
      CHECK(array.elem<int>(1, 0, 0) == 5);
      CHECK(array.elem<int>(1, 0, 3) == 8);
    }
    SUBCASE("operator[]") {
      SUBCASE("items access") {
        CHECK(array[0, 0, 0] == py::Int{1});
        CHECK(array[0, 0, 2] == py::Int{3});
        CHECK(array[1, 0, 0] == py::Int{5});
        CHECK(array[1, 0, 3] == py::Int{8});

        array[1, 0, 1] = py::Int{9};
        CHECK(array[1, 0, 1] == py::Int{9});
        CHECK_THROWS_MSG(
            (array[0, 0, 4] = py::Int{10}),
            py::ErrorException,
            "IndexError: index 4 is out of bounds for axis 2 with size 4");
        CHECK_THROWS_MSG(
            (array[1, 2, 3, 4] = py::Int{10}),
            py::ErrorException,
            "IndexError: too many indices for array: array is 3-dimensional, "
            "but 4 were indexed");
      }
      SUBCASE("slices access") {
        const auto slice2D = py::expect<py::NDArray>(array[1]);
        CHECK(slice2D.kind() == data::kind_of<int>);
        CHECK(slice2D.rank() == 2);
        CHECK_RANGE_EQ(slice2D.shape(), std::array{1, 4});
        CHECK(slice2D[0, 0] == py::Int{5});
        CHECK(slice2D[0, 1] == py::Int{6});
        CHECK(slice2D[0, 2] == py::Int{7});
        CHECK(slice2D[0, 3] == py::Int{8});
        CHECK_THROWS_MSG(
            py::repr(array[3]),
            py::ErrorException,
            "IndexError: index 3 is out of bounds for axis 0 with size 2");

        const auto slice1D = py::expect<py::NDArray>(slice2D[0]);
        CHECK(slice1D.kind() == data::kind_of<int>);
        CHECK(slice1D.rank() == 1);
        CHECK_RANGE_EQ(slice1D.shape(), std::array{4});
        CHECK(slice1D[0] == py::Int{5});
        CHECK(slice1D[1] == py::Int{6});
        CHECK(slice1D[2] == py::Int{7});
        CHECK(slice1D[3] == py::Int{8});
        CHECK_THROWS_MSG(
            py::repr(slice2D[1]),
            py::ErrorException,
            "IndexError: index 1 is out of bounds for axis 0 with size 1");

        slice1D[1] = py::Int{10};
        CHECK(slice1D[1] == py::Int{10});
        CHECK(slice2D[0, 1] == py::Int{10});
        CHECK(array[1, 0, 1] == py::Int{10});
        CHECK_THROWS_MSG(
            slice1D[5] = py::Int{11},
            py::ErrorException,
            "IndexError: index 5 is out of bounds for axis 0 with size 4");

        array[0] = slice2D;
        CHECK(array[0, 0, 0] == py::Int{5});
        CHECK(array[0, 0, 1] == py::Int{10});
        CHECK(array[0, 0, 2] == py::Int{7});
        CHECK(array[0, 0, 3] == py::Int{8});
        CHECK(array[1, 0, 0] == py::Int{5});
        CHECK(array[1, 0, 1] == py::Int{10});
        CHECK(array[1, 0, 2] == py::Int{7});
        CHECK(array[1, 0, 3] == py::Int{8});
        CHECK_THROWS_MSG(
            py::repr(array[3] = slice2D),
            py::ErrorException,
            "IndexError: index 3 is out of bounds for axis 0 with size 2");
      }
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("NDArrays from Python") {
  REQUIRE(testing::interpreter().exec(R"PY(
    import numpy as np
    array = np.array([
      [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9]
      ],
      [
        [10, 11, 12],
        [13, 14, 15],
        [16, 17, 18]
      ]
    ])
    slice2D = array[1]
    slice1D = array[1, 1:3, 0]
    complex_array = np.array([1, 2, 3], dtype=np.complex64)
  )PY"));

  // Here I'll convert everything to plain list to save some lines.
  const auto array =
      py::expect<py::NDArray>(testing::interpreter().globals()["array"]);
  CHECK(array.kind() == data::kind_of<int64_t>); // Weird NumPy behavior.
  REQUIRE(array.rank() == 3);
  REQUIRE_RANGE_EQ(array.shape(), std::array{2, 3, 3});
  CHECK(py::List{array[0, 0]} == py::make_list(1, 2, 3));
  CHECK(py::List{array[0, 1]} == py::make_list(4, 5, 6));
  CHECK(py::List{array[0, 2]} == py::make_list(7, 8, 9));
  CHECK(py::List{array[1, 0]} == py::make_list(10, 11, 12));
  CHECK(py::List{array[1, 1]} == py::make_list(13, 14, 15));
  CHECK(py::List{array[1, 2]} == py::make_list(16, 17, 18));

  const auto slice2D =
      py::expect<py::NDArray>(testing::interpreter().globals()["slice2D"]);
  REQUIRE(slice2D.rank() == 2);
  REQUIRE_RANGE_EQ(slice2D.shape(), std::array{3, 3});
  CHECK(py::List{slice2D[0]} == py::make_list(10, 11, 12));
  CHECK(py::List{slice2D[1]} == py::make_list(13, 14, 15));
  CHECK(py::List{slice2D[2]} == py::make_list(16, 17, 18));

  const auto slice1D =
      py::expect<py::NDArray>(testing::interpreter().globals()["slice1D"]);
  REQUIRE(slice1D.rank() == 1);
  REQUIRE_RANGE_EQ(slice1D.shape(), std::array{2});
  CHECK(py::List{slice1D} == py::make_list(13, 16));

  // There is no intention to all NumPy types, for example complex numbers.
  const auto complex_array = py::expect<py::NDArray>(
      testing::interpreter().globals()["complex_array"]);
  CHECK_THROWS_MSG(static_cast<void>(complex_array.kind()),
                   py::ErrorException,
                   "TypeError: Unsupported NumPy type");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
