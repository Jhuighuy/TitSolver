/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
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

TEST_CASE("data::DataKind") {
  SUBCASE("correct") {
    const data::DataKind kind{data::DataKind::ID::float32};
    CHECK(kind.id() == data::DataKind::ID::float32);
    CHECK(kind.name() == "float32_t");
    CHECK(kind.width() == 4);
  }
  SUBCASE("incorrect") {
    CHECK_THROWS_MSG(data::DataKind{data::DataKind::ID{0}},
                     Exception,
                     "Invalid data kind ID: 0.");
    CHECK_THROWS_MSG(data::DataKind{data::DataKind::ID{137}},
                     Exception,
                     "Invalid data kind ID: 137.");
  }
}

TEST_CASE("data::kind_of") {
  CHECK(data::kind_of<int16_t>.id() == data::DataKind::ID::int16);
  CHECK(data::kind_of<float32_t>.id() == data::DataKind::ID::float32);
  CHECK(data::kind_of<uint64_t>.id() == data::DataKind::ID::uint64);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::DataType") {
  SUBCASE("correct") {
    SUBCASE("scalar") {
      const data::DataType type{data::kind_of<float32_t>};
      CHECK(type.kind() == data::kind_of<float32_t>);
      CHECK(type.rank() == data::DataRank::scalar);
      CHECK(type.dim() == 1);
      CHECK(type.width() == 4);
      CHECK(type.name() == "float32_t");
    }
    SUBCASE("vector") {
      const data::DataType type{data::kind_of<float64_t>,
                                data::DataRank::vector,
                                2};
      CHECK(type.kind() == data::kind_of<float64_t>);
      CHECK(type.rank() == data::DataRank::vector);
      CHECK(type.dim() == 2);
      CHECK(type.width() == 2 * 8);
      CHECK(type.name() == "Vec<float64_t, 2>");
    }
    SUBCASE("matrix") {
      const data::DataType type{data::kind_of<int16_t>,
                                data::DataRank::matrix,
                                3};
      CHECK(type.kind() == data::kind_of<int16_t>);
      CHECK(type.rank() == data::DataRank::matrix);
      CHECK(type.dim() == 3);
      CHECK(type.width() == 3 * 3 * 2);
      CHECK(type.name() == "Mat<int16_t, 3>");
    }
  }
  SUBCASE("incorrect") {
    SUBCASE("invalid rank") {
      CHECK_THROWS_MSG(
          data::DataType(data::kind_of<float32_t>, data::DataRank{137}, 3),
          Exception,
          "Invalid data type rank: 137.");
    }
    SUBCASE("invalid dim") {
      CHECK_THROWS_MSG(
          data::DataType(data::kind_of<float32_t>, data::DataRank::vector, 0),
          Exception,
          "Dimensionality must be positive, but is 0.");
    }
    SUBCASE("invalid scalar dim") {
      CHECK_THROWS_MSG(
          data::DataType(data::kind_of<float32_t>, data::DataRank::scalar, 2),
          Exception,
          "Dimensionality of a scalar must be 1, but is 2.");
    }
  }
  SUBCASE("ID") {
    SUBCASE("to ID") {
      CHECK(data::type_of<Mat<float32_t, 3>>.id() == 0x030209);
    }
    SUBCASE("from ID") {
      SUBCASE("valid") {
        CHECK(data::DataType{0x030209} == data::type_of<Mat<float32_t, 3>>);
      }
      SUBCASE("invalid") {
        CHECK_THROWS_MSG(data::DataType{0x1337}, Exception, "Invalid");
      }
    }
  }
}

TEST_CASE("data::type_of") {
  SUBCASE("scalar") {
    const auto type = data::type_of<float32_t>;
    CHECK(type.kind() == data::kind_of<float32_t>);
    CHECK(type.rank() == data::DataRank::scalar);
    CHECK(type.dim() == 1);
  }
  SUBCASE("vector") {
    const auto type = data::type_of<Vec<int16_t, 7>>;
    CHECK(type.kind() == data::kind_of<int16_t>);
    CHECK(type.rank() == data::DataRank::vector);
    CHECK(type.dim() == 7);
  }
  SUBCASE("matrix") {
    const auto type = data::type_of<Mat<float64_t, 5>>;
    CHECK(type.kind() == data::kind_of<float64_t>);
    CHECK(type.rank() == data::DataRank::matrix);
    CHECK(type.dim() == 5);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
