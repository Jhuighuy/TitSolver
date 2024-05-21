/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/vec.hpp"

#include "tit/data/type.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::DataType") {
  SUBCASE("correct") {
    SUBCASE("known") {
      const data::DataType type{data::DataType::Kind::float32,
                                data::DataType::Rank::scalar,
                                1};
      CHECK(type.kind() == data::DataType::Kind::float32);
      CHECK(type.rank() == data::DataType::Rank::scalar);
      CHECK(type.dim() == 1);
    }
    SUBCASE("unknown kind") {
      const data::DataType type{data::DataType::Kind::unknown,
                                data::DataType::Rank::scalar,
                                3};
      CHECK(type.kind() == data::DataType::Kind::unknown);
    }
  }
  SUBCASE("incorrect") {
    SUBCASE("invalid kind") {
      CHECK_THROWS_MSG(data::DataType(data::DataType::Kind{137},
                                      data::DataType::Rank::scalar,
                                      3),
                       Exception,
                       "Invalid data type kind: 137.");
    }
    SUBCASE("invalid rank") {
      CHECK_THROWS_MSG(data::DataType(data::DataType::Kind::float32,
                                      data::DataType::Rank{137},
                                      3),
                       Exception,
                       "Invalid data type rank: 137.");
    }
    SUBCASE("invalid dim") {
      CHECK_THROWS_MSG(data::DataType(data::DataType::Kind::float32,
                                      data::DataType::Rank::scalar,
                                      0),
                       Exception,
                       "Dimensionality must be positive, but is 0.");
    }
    SUBCASE("invalid scalar dim") {
      CHECK_THROWS_MSG(data::DataType(data::DataType::Kind::float32,
                                      data::DataType::Rank::scalar,
                                      2),
                       Exception,
                       "Dimensionality of a scalar must be 1, but is 2.");
    }
  }
}

TEST_CASE("data::DataType::id") {
  SUBCASE("to ID") {
    const data::DataType type{data::DataType::Kind::float32,
                              data::DataType::Rank::matrix,
                              3};
    CHECK(type.id() == 0x030209);
  }
  SUBCASE("from ID") {
    SUBCASE("valid") {
      const data::DataType type{0x030209};
      CHECK(type.kind() == data::DataType::Kind::float32);
      CHECK(type.rank() == data::DataType::Rank::matrix);
      CHECK(type.dim() == 3);
    }
    SUBCASE("invalid") {
      CHECK_THROWS_MSG(data::DataType{0x1337}, Exception, "Invalid");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::data_type_of") {
  SUBCASE("scalar") {
    const auto type = data::data_type_of<float32_t>;
    CHECK(type.kind() == data::DataType::Kind::float32);
    CHECK(type.rank() == data::DataType::Rank::scalar);
    CHECK(type.dim() == 1);
  }
  SUBCASE("vector") {
    const auto type = data::data_type_of<Vec<int16_t, 7>>;
    CHECK(type.kind() == data::DataType::Kind::int16);
    CHECK(type.rank() == data::DataType::Rank::vector);
    CHECK(type.dim() == 7);
  }
  SUBCASE("matrix") {
    const auto type = data::data_type_of<Mat<float64_t, 5>>;
    CHECK(type.kind() == data::DataType::Kind::float64);
    CHECK(type.rank() == data::DataType::Rank::matrix);
    CHECK(type.dim() == 5);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
