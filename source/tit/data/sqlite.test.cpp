/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <filesystem>
#include <numbers>
#include <string>
#include <tuple>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/sys/utils.hpp"
#include "tit/core/utils.hpp"

#include "tit/data/sqlite.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::sqlite::Database") {
  const std::filesystem::path file_name{"test.db"};
  const std::filesystem::path invalid_file_name{"/invalid/path/to/file.db"};
  SUBCASE("success") {
    SUBCASE("create in-memory") {
      const data::sqlite::Database db{":memory:"};
      CHECK(db.base() != nullptr);
      CHECK(db.path().empty());
    }
    SUBCASE("create file") {
      if (std::filesystem::exists(file_name)) {
        // Remove the file if it already exists.
        REQUIRE(std::filesystem::remove(file_name));
      }
      const data::sqlite::Database db{file_name};
      CHECK(std::filesystem::exists(file_name));
      CHECK(db.base() != nullptr);
      CHECK(db.path().filename() == file_name);
    }
    SUBCASE("open existing") {
      // Should exist due to the previous test.
      REQUIRE(std::filesystem::exists(file_name));
      const data::sqlite::Database db{file_name};
      CHECK(db.base() != nullptr);
      CHECK(db.path().filename() == file_name);
    }
  }
  SUBCASE("failure") {
    SUBCASE("cannot create") {
      REQUIRE(!std::filesystem::exists(invalid_file_name));
      CHECK_THROWS_MSG(data::sqlite::Database{invalid_file_name},
                       Exception,
                       "unable to open database file");
    }
    // Note: we cannot check for corrupt database files, since SQLite checks
    // verifies the database file integrity on execution, not on opening.
  }
}

TEST_CASE("data::sqlite::Database::execute") {
  SUBCASE("success") {
    const data::sqlite::Database db{":memory:"};
    db.execute("CREATE TABLE IF NOT EXISTS test (id INTEGER PRIMARY KEY)");
    db.execute("SELECT * FROM test");
  }
  SUBCASE("failure") {
    SUBCASE("invalid SQL") {
      const data::sqlite::Database db{":memory:"};
      CHECK_THROWS_MSG(db.execute("INVALID SQL"), Exception, "syntax error");
    }
    SUBCASE("invalid operation") {
      const data::sqlite::Database db{":memory:"};
      CHECK_THROWS_MSG( //
          db.execute(R"SQL(
            CREATE TABLE test (id INTEGER PRIMARY KEY);
            INSERT INTO test (id) VALUES ("not an integer");
          )SQL"),
          Exception,
          "datatype mismatch");
    }
    SUBCASE("invalid database") {
      // Our current executable definitely is not a valid database file.
      const data::sqlite::Database db{exe_path()};
      CHECK_THROWS_MSG(db.execute("SELECT * FROM test"),
                       Exception,
                       "file is not a database");
    }
  }
}

TEST_CASE("data::sqlite::Database::last_insert_row_id") {
  const data::sqlite::Database db{":memory:"};
  db.execute(R"SQL(
    CREATE TABLE IF NOT EXISTS test (
      id    INTEGER PRIMARY KEY,
      value TEXT
    );
    INSERT INTO test (value) VALUES ("first");
    INSERT INTO test (value) VALUES ("second");
  )SQL");
  CHECK(db.last_insert_row_id() == 2);
  db.execute(R"SQL(
    DELETE FROM test WHERE id = 1;
    INSERT INTO test (value) VALUES ("third");
  )SQL");
  CHECK(db.last_insert_row_id() == 3);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::sqlite::Statement") {
  data::sqlite::Database db{":memory:"};
  db.execute("CREATE TABLE test (id INTEGER PRIMARY KEY)");
  SUBCASE("success") {
    const data::sqlite::Statement s{db, "INSERT INTO test (id) VALUES (?)"};
    CHECK(s.base() != nullptr);
  }
  SUBCASE("failure") {
    SUBCASE("invalid SQL") {
      CHECK_THROWS_MSG(data::sqlite::Statement(db, "INVALID SQL"),
                       Exception,
                       "syntax error");
    }
    SUBCASE("invalid operation") {
      CHECK_THROWS_MSG(data::sqlite::Statement(db, "CREATE TABLE test ()"),
                       Exception,
                       "table test already exists");
    }
  }
}

TEST_CASE("data::sqlite::Statement::step") {
  data::sqlite::Database db{":memory:"};
  db.execute(R"SQL(
    CREATE TABLE IF NOT EXISTS Constants (
      id    INTEGER PRIMARY KEY,
      name  TEXT,
      value REAL
    );
    INSERT INTO Constants VALUES (1, "pi", 3.14);
    INSERT INTO Constants VALUES (2, "e", 2.71);
    INSERT INTO Constants VALUES (3, "phi", 1.61);
  )SQL");
  SUBCASE("insert rows") {
    data::sqlite::Statement statement{db, R"SQL(
      INSERT INTO Constants (id, name, value) VALUES (?, ?, ?)
    )SQL"};
    statement.bind(4, "sqrt2", 1.41);
    CHECK_FALSE(statement.step());
  }
  SUBCASE("delete rows") {
    data::sqlite::Statement statement{db, R"SQL(
      DELETE FROM Constants WHERE value < ?
    )SQL"};
    statement.bind(2.0);
    CHECK_FALSE(statement.step());
  }
  SUBCASE("select rows") {
    data::sqlite::Statement statement{db, R"SQL(
      SELECT name FROM Constants
    )SQL"};
    REQUIRE(statement.step());
    CHECK(statement.column<std::string>() == "pi");
    CHECK(statement.step());
    CHECK(statement.column<std::string>() == "e");
    CHECK(statement.step());
    CHECK(statement.column<std::string>() == "phi");
    CHECK_FALSE(statement.step());
  }
}

TEST_CASE("data::sqlite::Statement::run") {
  data::sqlite::Database db{":memory:"};
  db.execute(R"SQL(
    CREATE TABLE IF NOT EXISTS Constants (
      id    INTEGER PRIMARY KEY,
      name  TEXT,
      value REAL
    ) STRICT;
    INSERT INTO Constants VALUES (1, "pi", 3.14);
    INSERT INTO Constants VALUES (2, "e", 2.71);
    INSERT INTO Constants VALUES (3, "phi", 1.61);
  )SQL");
  data::sqlite::Statement statement{db, R"SQL(
    INSERT INTO Constants (id, name, value) VALUES (?, ?, ?)
  )SQL"};
  SUBCASE("success") {
    SUBCASE("bind, run") {
      statement.bind(4, "sqrt2", 1.41);
      statement.run();
      statement.bind(5, "sqrt3", 1.73);
      statement.run();
    }
    SUBCASE("bind-run") {
      statement.run(4, "sqrt2", 1.41);
      statement.run(5, "sqrt3", 1.73);
    }
  }
  SUBCASE("failure") {
    SUBCASE("invalid argument type") {
      // Note: this failure only happens because we are using the `STRICT`
      // keyword in the table definition. Without it, SQLite would allow us to
      // bind (and, therefore, store) a string in a REAL column.
      statement.bind(4, "sqrt2", "one point forty one");
      CHECK_THROWS_MSG(statement.run(),
                       Exception,
                       "cannot store TEXT value in REAL column");
    }
  }
}

TEST_CASE("data::sqlite::Statement::column") {
  using data::sqlite::BlobView;
  using Blob = std::vector<byte_t>;
  data::sqlite::Database db{":memory:"};
  db.execute(R"SQL(
    CREATE TABLE IF NOT EXISTS Constants (
      id     INTEGER PRIMARY KEY,
      name   TEXT,
      approx REAL,
      exact  BLOB
    ) STRICT;
  )SQL");
  { // Insert some values.
    data::sqlite::Statement statement{db, R"SQL(
      INSERT INTO Constants (id, name, approx, exact) VALUES (?, ?, ?, ?)
    )SQL"};
    statement.run(1, "pi", 3.14, to_byte_array(std::numbers::pi_v<float>));
    statement.run(2, "e", 2.71, to_byte_array(std::numbers::e_v<long double>));
    statement.run(3, "phi", 1.61, to_byte_array(std::numbers::phi_v<double>));
  }
  { // Select and check the values.
    data::sqlite::Statement statement{db, R"SQL(
      SELECT * FROM Constants
    )SQL"};
    REQUIRE(statement.step());
    CHECK(statement.columns<size_t, std::string, real_t, Blob>() ==
          std::tuple{1, "pi", 3.14, to_bytes(std::numbers::pi_v<float>)});
    REQUIRE(statement.step());
    CHECK(statement.columns<size_t, std::string_view, real_t, Blob>() ==
          std::tuple{2, "e", 2.71, to_bytes(std::numbers::e_v<long double>)});
    REQUIRE(statement.step());
    {
      const auto [id, name, approx, exact] =
          statement.columns<size_t, std::string, real_t, BlobView>();
      CHECK(id == 3);
      CHECK(name == "phi");
      CHECK(approx == 1.61);
      CHECK_RANGE_EQ(exact, to_byte_array(std::numbers::phi_v<double>));
    }
    CHECK_FALSE(statement.step());
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
