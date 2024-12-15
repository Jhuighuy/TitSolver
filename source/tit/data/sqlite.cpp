/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <bit>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>

#include <sqlite3.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/log.hpp"
#include "tit/core/utils.hpp"

#include "tit/data/sqlite.hpp"

namespace tit::data::sqlite {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

auto error_message(int status, sqlite3* db = nullptr) -> std::string {
  const char* error_message = nullptr;
  if (db != nullptr) error_message = sqlite3_errmsg(db);
  if (error_message == nullptr) error_message = sqlite3_errstr(status);
  return error_message;
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Database::Database(const std::filesystem::path& path) {
  sqlite3* db = nullptr;
  const auto status = sqlite3_open_v2( //
      path.c_str(),
      &db,
      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
      nullptr);
  if (status == SQLITE_OK) {
    db_.reset(db);
    return;
  }

  TIT_THROW("SQLite database open failed ({}): {}",
            status,
            error_message(status, db));
}

void Database::Closer_::operator()(sqlite3* db) {
  const auto status = sqlite3_close_v2(db);
  if (status == SQLITE_OK) return;

  // Let's not throw in destructors.
  TIT_ERROR("SQLite database close failed ({}): {}",
            status,
            error_message(status, db));
}

auto Database::base() const noexcept -> sqlite3* {
  TIT_ASSERT(db_ != nullptr, "Database was not opened!");
  return db_.get();
}

auto Database::path() const -> std::filesystem::path {
  const auto* const path = sqlite3_db_filename(base(), "main");
  if (path != nullptr) return path;
  TIT_THROW("Could not get database path!");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Database::execute(const char* sql) const {
  TIT_ASSERT(sql != nullptr, "SQL statement is null!");

  char* error_message = nullptr;
  const auto status = sqlite3_exec(base(),
                                   sql,
                                   /*callback=*/nullptr,
                                   /*callback_arg=*/nullptr,
                                   &error_message);
  if (status == SQLITE_OK) return;

  std::string error_message_str{error_message};
  sqlite3_free(error_message);
  TIT_THROW("SQLite operation '{}' failed ({}): {}",
            sql,
            status,
            error_message_str);
}

auto Database::last_insert_row_id() const -> RowID {
  return sqlite3_last_insert_rowid(base());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Statement::Statement(Database& db, std::string_view sql) : db_{&db} {
  TIT_ASSERT(!sql.empty(), "SQL statement is null!");
  TIT_ASSERT(sql.size() <= std::numeric_limits<int>::max(), "SQL is too big!");

  sqlite3_stmt* stmt = nullptr;
  const auto status = sqlite3_prepare_v3(db_->base(),
                                         sql.data(),
                                         static_cast<int>(sql.size()),
                                         SQLITE_PREPARE_PERSISTENT,
                                         &stmt,
                                         /*pzTail=*/nullptr);
  if (status == SQLITE_OK) {
    stmt_.reset(stmt);
    state_ = State_::prepared;
    return;
  }

  TIT_THROW("SQLite statement '{}' prepare failed ({}): {}",
            sql,
            status,
            error_message(status, db_->base()));
}

void Statement::Finalizer_::operator()(sqlite3_stmt* stmt) {
  const auto status = sqlite3_finalize(stmt);
  if (status == SQLITE_OK) return;

  // `sqlite3_finalize` returns an error code if any usage of the statement
  // resulted in an error, so we've must have already thrown an exception.
  // So, let's just log the error here and return peacefully.
  TIT_ERROR("SQLite statement close failed ({}): {}",
            status,
            error_message(status));
}

auto Statement::base() const noexcept -> sqlite3_stmt* {
  TIT_ASSERT(stmt_.get() != nullptr, "Statement was moved away!");
  return stmt_.get();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Statement::num_params_() const -> size_t {
  return static_cast<size_t>(sqlite3_bind_parameter_count(base()));
}

void Statement::bind_(size_t index, int64_t value) const {
  TIT_ASSERT(state_ == State_::prepared, "Statement is not prepared!");
  TIT_ASSERT(in_range(index, 1, num_params_()), "Param index is out of range!");

  const auto status =
      sqlite3_bind_int64(base(), static_cast<int>(index), value);
  if (status == SQLITE_OK) return;

  TIT_THROW("SQLite statement bind integer argument #{} failed ({}): {}",
            index,
            status,
            error_message(status, db_->base()));
}

void Statement::bind_(size_t index, float64_t value) const {
  TIT_ASSERT(state_ == State_::prepared, "Statement is not prepared!");
  TIT_ASSERT(in_range(index, 1, num_params_()), "Param index is out of range!");

  const auto status =
      sqlite3_bind_double(base(), static_cast<int>(index), value);
  if (status == SQLITE_OK) return;

  TIT_THROW("SQLite statement bind real argument #{} failed ({}): {}",
            index,
            status,
            error_message(status, db_->base()));
}

void Statement::bind_(size_t index, std::string_view value) const {
  TIT_ASSERT(state_ == State_::prepared, "Statement is not prepared!");
  TIT_ASSERT(in_range(index, 1, num_params_()), "Param index is out of range!");
  TIT_ASSERT(value.size() <= std::numeric_limits<int>::max(),
             "Statement string argument is too large!");

  const auto status = sqlite3_bind_text(base(),
                                        static_cast<int>(index),
                                        value.data(),
                                        static_cast<int>(value.size()),
                                        SQLITE_STATIC);
  if (status == SQLITE_OK) return;

  TIT_THROW("SQLite statement bind text argument #{} failed ({}): {}",
            index,
            status,
            error_message(status, db_->base()));
}

void Statement::bind_(size_t index, BlobView value) const {
  TIT_ASSERT(state_ == State_::prepared, "Statement is not prepared!");
  TIT_ASSERT(in_range(index, 1, num_params_()), "Param index is out of range!");
  TIT_ASSERT(value.size() <= std::numeric_limits<int>::max(),
             "Statement blob argument is too large!");

  const auto status = sqlite3_bind_blob(base(),
                                        static_cast<int>(index),
                                        value.data(),
                                        static_cast<int>(value.size()),
                                        SQLITE_STATIC);
  if (status == SQLITE_OK) return;

  TIT_THROW("SQLite statement bind blob argument #{} failed ({}): {}",
            index,
            status,
            error_message(status, db_->base()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Statement::step() -> bool {
  TIT_ASSERT(is_any_of(state_, State_::prepared, State_::executing),
             "Statement was already executed!");

  const auto status = sqlite3_step(base());
  if (status == SQLITE_DONE) {
    state_ = State_::finished;
    return false;
  }
  if (status == SQLITE_ROW) {
    state_ = State_::executing;
    return true;
  }

  TIT_THROW("SQLite statement step failed ({}): {}",
            status,
            error_message(status, db_->base()));
}

void Statement::reset() {
  TIT_ASSERT(state_ == State_::finished, "Statement was not finished!");

  sqlite3_reset(base());
  state_ = State_::prepared;
}

void Statement::run() {
  TIT_ASSERT(is_any_of(state_, State_::prepared, State_::finished),
             "Statement must be either prepared or finished!");

  const auto more_rows = step();
  TIT_ASSERT(!more_rows, "Statement must finish in a single step!");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Statement::num_columns_() const -> size_t {
  TIT_ASSERT(state_ == State_::executing, "Statement is not executing!");

  const auto count = sqlite3_column_count(base());
  if (count > 0) return static_cast<size_t>(count);

  TIT_THROW("SQLite statement's column count query failed: {}",
            error_message(sqlite3_errcode(db_->base()), db_->base()));
}

auto Statement::column_type_(size_t index) const -> int {
  TIT_ASSERT(state_ == State_::executing, "Statement is not executing!");
  TIT_ASSERT(index < num_columns_(), "Column index is out of range!");

  const auto type = sqlite3_column_type(base(), static_cast<int>(index));
  if (type > 0) return type;

  TIT_THROW("SQLite statement's column type #{} query failed: {}",
            index,
            error_message(sqlite3_errcode(db_->base()), db_->base()));
}

auto Statement::column_int_(size_t index) const -> int64_t {
  TIT_ASSERT(state_ == State_::executing, "Statement is not executing!");
  TIT_ASSERT(index < num_columns_(), "Column index is out of range!");
  TIT_ASSERT(column_type_(index) == SQLITE_INTEGER, "Column type mismatch!");

  return sqlite3_column_int64(base(), static_cast<int>(index));
}

auto Statement::column_real_(size_t index) const -> float64_t {
  TIT_ASSERT(state_ == State_::executing, "Statement is not executing!");
  TIT_ASSERT(index < num_columns_(), "Column index is out of range!");
  TIT_ASSERT(column_type_(index) == SQLITE_FLOAT, "Column type mismatch!");

  return sqlite3_column_double(base(), static_cast<int>(index));
}

auto Statement::column_text_(size_t index) const -> std::string_view {
  TIT_ASSERT(state_ == State_::executing, "Statement is not executing!");
  TIT_ASSERT(index < num_columns_(), "Column index is out of range!");
  TIT_ASSERT(column_type_(index) == SQLITE_TEXT, "Column type mismatch!");

  const auto index_int = static_cast<int>(index);
  const auto* const value_uchar_ptr = sqlite3_column_text(base(), index_int);
  if (value_uchar_ptr == nullptr) {
    TIT_THROW("SQLite statement failed to retrieve text column data #{}: {}",
              index,
              error_message(sqlite3_errcode(db_->base()), db_->base()));
  }

  const auto num_bytes_int = sqlite3_column_bytes(base(), index_int);
  if (num_bytes_int < 0) {
    TIT_THROW("SQLite statement failed to retrieve text column size #{}: {}",
              index,
              error_message(sqlite3_errcode(db_->base()), db_->base()));
  }

  return {std::bit_cast<const char*>(value_uchar_ptr),
          static_cast<size_t>(num_bytes_int)};
}

auto Statement::column_blob_(size_t index) const -> BlobView {
  TIT_ASSERT(state_ == State_::executing, "Statement is not executing!");
  TIT_ASSERT(index < num_columns_(), "Column index is out of range!");
  TIT_ASSERT(column_type_(index) == SQLITE_BLOB, "Column type mismatch!");

  const auto index_int = static_cast<int>(index);
  const auto* const value_void_ptr = sqlite3_column_blob(base(), index_int);
  if (value_void_ptr == nullptr) {
    TIT_THROW("SQLite statement failed to retrieve blob column data #{}: {}",
              index,
              error_message(sqlite3_errcode(db_->base()), db_->base()));
  }

  const auto num_bytes_int = sqlite3_column_bytes(base(), index_int);
  if (num_bytes_int < 0) {
    TIT_THROW("SQLite statement failed to retrieve blob column size #{}: {}",
              index,
              error_message(sqlite3_errcode(db_->base()), db_->base()));
  }

  return {static_cast<const byte_t*>(value_void_ptr),
          static_cast<size_t>(num_bytes_int)};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data::sqlite
