/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <filesystem>
#include <numbers>
#include <ranges>
#include <string>
#include <tuple>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/range_utils.hpp"
#include "tit/core/sys/utils.hpp"

#include "tit/data/sqlite.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Blob = std::vector<byte_t>;

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
    SUBCASE("open readonly") {
      // Should exist due to the previous test.
      REQUIRE(std::filesystem::exists(file_name));
      const data::sqlite::Database db{file_name, /*read_only=*/true};
      CHECK_THROWS_MSG(db.execute("CREATE TABLE test (id INTEGER PRIMARY KEY)"),
                       Exception,
                       "attempt to write a readonly database");
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

TEST_CASE("data::sqlite::BlobReader") {
  data::sqlite::Database db{":memory:"};
  db.execute(R"SQL(
    CREATE TABLE IF NOT EXISTS Constants (
      id    INTEGER PRIMARY KEY,
      value BLOB
    ) STRICT;
  )SQL");
  data::sqlite::Statement statement{db, R"SQL(
    INSERT INTO Constants (id, value) VALUES (?, ?)
  )SQL"};
  statement.run(1, to_byte_array(std::numbers::pi_v<float>));
  statement.run(2, to_byte_array(std::numbers::e_v<long double>));
  statement.run(3, to_byte_array(std::numbers::phi_v<double>));
  SUBCASE("success") {
    SUBCASE("exact size") {
      auto reader = data::sqlite::make_blob_reader(db, "Constants", "value", 1);
      std::vector<byte_t> result(sizeof(float));
      REQUIRE(reader->read(result) == sizeof(float));
      CHECK(result == to_bytes(std::numbers::pi_v<float>));
      CHECK(reader->read(result) == 0);
    }
    SUBCASE("smaller size") {
      auto reader = data::sqlite::make_blob_reader(db, "Constants", "value", 2);
      std::vector<byte_t> result(sizeof(long double) / 2);
      REQUIRE(reader->read(result) == sizeof(long double) / 2);
      CHECK(result <= to_bytes(std::numbers::e_v<long double>));
      CHECK(reader->read(result) == sizeof(long double) / 2);
      CHECK(reader->read(result) == 0);
    }
    SUBCASE("larger size") {
      auto reader = data::sqlite::make_blob_reader(db, "Constants", "value", 3);
      std::vector<byte_t> result(sizeof(double) * 2);
      REQUIRE(reader->read(result) == sizeof(double));
      CHECK(result >= to_bytes(std::numbers::phi_v<double>));
      CHECK(reader->read(result) == 0);
    }
  }
  SUBCASE("failure") {
    SUBCASE("invalid table name") {
      CHECK_THROWS_MSG(
          data::sqlite::make_blob_reader(db, "invalid", "value", 1),
          Exception,
          "no such table");
    }
    SUBCASE("invalid column name") {
      CHECK_THROWS_MSG(
          data::sqlite::make_blob_reader(db, "Constants", "invalid", 1),
          Exception,
          "no such column");
    }
    SUBCASE("invalid row ID") {
      CHECK_THROWS_MSG(
          data::sqlite::make_blob_reader(db, "Constants", "value", 100),
          Exception,
          "no such row");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::sqlite::BlobWriter") {
  data::sqlite::Database db{":memory:"};
  db.execute(R"SQL(
    CREATE TABLE IF NOT EXISTS Constants (
      id    INTEGER PRIMARY KEY,
      value BLOB
    ) STRICT;
    INSERT INTO Constants (id) VALUES (1);
  )SQL");
  SUBCASE("success") {
    SUBCASE("single write") {
      const auto run_writer = [&db](auto value) {
        data::sqlite::make_blob_writer(db, "Constants", "value", 1)
            ->write(to_byte_array(value));

        data::sqlite::Statement statement{db, R"SQL(
          SELECT value FROM Constants WHERE id = 1
        )SQL"};
        REQUIRE(statement.step());
        CHECK(statement.column<Blob>() == to_bytes(value));
      };
      SUBCASE("write") {
        run_writer(std::numbers::pi);
      }
      SUBCASE("overwrite") {
        run_writer(std::numbers::pi);
        run_writer(std::numbers::e_v<float>);
        run_writer(std::array{std::numbers::phi, std::numbers::sqrt2});
      }
    }
    SUBCASE("empty") {
      data::sqlite::make_blob_writer(db, "Constants", "value", 1)->flush();
      data::sqlite::Statement statement{db, R"SQL(
        SELECT value FROM Constants WHERE id = 1
      )SQL"};
      REQUIRE(statement.step());
      CHECK(statement.column<Blob>().empty());
    }
    SUBCASE("multiple writes") {
      auto writer = data::sqlite::make_blob_writer(db, "Constants", "value", 1);
      writer->write(to_byte_array(std::numbers::pi_v<float>));
      writer->flush();
      writer->write(to_byte_array(std::numbers::e_v<long double>));
      writer->write(to_byte_array(std::numbers::phi_v<double>));
      writer->flush();
      data::sqlite::Statement statement{db, R"SQL(
        SELECT value FROM Constants WHERE id = 1
      )SQL"};
      REQUIRE(statement.step());
      CHECK_RANGE_EQ(statement.column<Blob>(),
                     std::vector{to_bytes(std::numbers::pi_v<float>),
                                 to_bytes(std::numbers::e_v<long double>),
                                 to_bytes(std::numbers::phi_v<double>)} |
                         std::views::join);
    }
  }
  SUBCASE("failure") {
    SUBCASE("SQL injection") {
      CHECK_THROWS_MSG(
          data::sqlite::make_blob_writer(db, "DROP TABLE", "value", 1),
          Exception,
          "Invalid table name");
      CHECK_THROWS_MSG(
          data::sqlite::make_blob_writer(db, "Constants", "DROP COLUMN", 1),
          Exception,
          "Invalid column name");
    }
    SUBCASE("invalid table name") {
      CHECK_THROWS_MSG(
          data::sqlite::make_blob_writer(db, "invalid", "value", 1)->flush(),
          Exception,
          "no such table");
    }
    SUBCASE("invalid column name") {
      CHECK_THROWS_MSG(
          data::sqlite::make_blob_writer(db, "Constants", "invalid", 1)
              ->flush(),
          Exception,
          "no such column");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
