/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <limits>
#include <span>
#include <string>
#include <string_view>

#include <sqlite3.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/print.hpp"
#include "tit/core/str.hpp"
#include "tit/core/type.hpp"
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

Database::Database(const std::filesystem::path& path, bool read_only) {
  // Open the database.
  sqlite3* db = nullptr;
  const auto flags = read_only ? SQLITE_OPEN_READONLY :
                                 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
  if (const auto status = sqlite3_open_v2(path.c_str(), &db, flags, nullptr);
      status != SQLITE_OK) {
    TIT_THROW("SQLite database open failed ({}): {}.",
              status,
              error_message(status, db));
  }
  db_.reset(db);

  // Execute a simple query to trigger the database integrity check.
  execute("SELECT 0 WHERE 0");
}

void Database::Closer_::operator()(sqlite3* db) {
  if (const auto status = sqlite3_close_v2(db); status != SQLITE_OK) {
    // Let's not throw in destructors.
    err("SQLite database close failed ({}): {}.",
        status,
        error_message(status, db));
  }
}

auto Database::base() const noexcept -> sqlite3* {
  TIT_ASSERT(db_ != nullptr, "Database was not opened!");
  return db_.get();
}

auto Database::path() const -> std::filesystem::path {
  const auto* const path = sqlite3_db_filename(base(), "main");
  if (path == nullptr) TIT_THROW("Could not get database path!");
  return path;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Database::execute(CStrView sql) const {
  char* error_message = nullptr;
  if (const auto status = sqlite3_exec(base(),
                                       sql.c_str(),
                                       /*callback=*/nullptr,
                                       /*callback_arg=*/nullptr,
                                       &error_message);
      status != SQLITE_OK) {
    std::string error_message_str{error_message};
    sqlite3_free(error_message);
    TIT_THROW("SQLite operation '{}' failed ({}): {}.",
              sql,
              status,
              error_message_str);
  }
}

auto Database::last_insert_row_id() const -> RowID {
  return sqlite3_last_insert_rowid(base());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Statement::Statement(Database& db, std::string_view sql) : db_{&db} {
  TIT_ASSERT(!sql.empty(), "SQL statement is null!");
  TIT_ASSERT(sql.size() <= std::numeric_limits<int>::max(), "SQL is too big!");

  sqlite3_stmt* stmt = nullptr;
  if (const auto status = sqlite3_prepare_v3(db_->base(),
                                             sql.data(),
                                             static_cast<int>(sql.size()),
                                             SQLITE_PREPARE_PERSISTENT,
                                             &stmt,
                                             /*pzTail=*/nullptr);
      status != SQLITE_OK) {
    TIT_THROW("SQLite statement '{}' prepare failed ({}): {}.",
              sql,
              status,
              error_message(status, db_->base()));
  }

  stmt_.reset(stmt);
  state_ = State_::prepared;
}

void Statement::Finalizer_::operator()(sqlite3_stmt* stmt) {
  if (const auto status = sqlite3_finalize(stmt); status != SQLITE_OK) {
    // `sqlite3_finalize` returns an error code if any usage of the statement
    // resulted in an error, so we've must have already thrown an exception.
    // So, let's just log the error here and return peacefully.
    err("SQLite statement close failed ({}): {}.",
        status,
        error_message(status));
  }
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

  if (const auto status =
          sqlite3_bind_int64(base(), static_cast<int>(index), value);
      status != SQLITE_OK) {
    TIT_THROW("SQLite statement bind integer argument #{} failed ({}): {}.",
              index,
              status,
              error_message(status, db_->base()));
  }
}

void Statement::bind_(size_t index, float64_t value) const {
  TIT_ASSERT(state_ == State_::prepared, "Statement is not prepared!");
  TIT_ASSERT(in_range(index, 1, num_params_()), "Param index is out of range!");

  if (const auto status =
          sqlite3_bind_double(base(), static_cast<int>(index), value);
      status != SQLITE_OK) {
    TIT_THROW("SQLite statement bind real argument #{} failed ({}): {}.",
              index,
              status,
              error_message(status, db_->base()));
  }
}

void Statement::bind_(size_t index, std::string_view value) const {
  TIT_ASSERT(state_ == State_::prepared, "Statement is not prepared!");
  TIT_ASSERT(in_range(index, 1, num_params_()), "Param index is out of range!");
  TIT_ASSERT(value.size() <= std::numeric_limits<int>::max(),
             "Statement string argument is too large!");

  if (const auto status = sqlite3_bind_text(base(),
                                            static_cast<int>(index),
                                            value.data(),
                                            static_cast<int>(value.size()),
                                            SQLITE_STATIC);
      status != SQLITE_OK) {
    TIT_THROW("SQLite statement bind text argument #{} failed ({}): {}.",
              index,
              status,
              error_message(status, db_->base()));
  }
}

void Statement::bind_(size_t index, BlobView value) const {
  TIT_ASSERT(state_ == State_::prepared, "Statement is not prepared!");
  TIT_ASSERT(in_range(index, 1, num_params_()), "Param index is out of range!");
  TIT_ASSERT(value.size() <= std::numeric_limits<int>::max(),
             "Statement blob argument is too large!");

  if (const auto status = sqlite3_bind_blob(base(),
                                            static_cast<int>(index),
                                            value.data(),
                                            static_cast<int>(value.size()),
                                            SQLITE_STATIC);
      status != SQLITE_OK) {
    TIT_THROW("SQLite statement bind blob argument #{} failed ({}): {}.",
              index,
              status,
              error_message(status, db_->base()));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Statement::step() -> bool {
  TIT_ASSERT(is_any_of(state_, State_::prepared, State_::executing),
             "Statement was already executed!");

  switch (const auto status = sqlite3_step(base()); status) {
    case SQLITE_DONE: state_ = State_::finished; return false;
    case SQLITE_ROW:  state_ = State_::executing; return true;
    default:
      TIT_THROW("SQLite statement step failed ({}): {}.",
                status,
                error_message(status, db_->base()));
  }
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
  if (count <= 0) {
    TIT_THROW("SQLite statement's column count query failed: {}.",
              error_message(sqlite3_errcode(db_->base()), db_->base()));
  }

  return static_cast<size_t>(count);
}

auto Statement::column_type_(size_t index) const -> int {
  TIT_ASSERT(state_ == State_::executing, "Statement is not executing!");
  TIT_ASSERT(index < num_columns_(), "Column index is out of range!");

  const auto type = sqlite3_column_type(base(), static_cast<int>(index));
  if (type <= 0) {
    TIT_THROW("SQLite statement's column type #{} query failed: {}.",
              index,
              error_message(sqlite3_errcode(db_->base()), db_->base()));
  }

  return type;
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
    TIT_THROW("SQLite statement failed to retrieve text column data #{}: {}.",
              index,
              error_message(sqlite3_errcode(db_->base()), db_->base()));
  }

  const auto num_bytes_int = sqlite3_column_bytes(base(), index_int);
  if (num_bytes_int < 0) {
    TIT_THROW("SQLite statement failed to retrieve text column size #{}: {}.",
              index,
              error_message(sqlite3_errcode(db_->base()), db_->base()));
  }

  return {safe_bit_ptr_cast<const char*>(value_uchar_ptr),
          static_cast<size_t>(num_bytes_int)};
}

auto Statement::column_blob_(size_t index) const -> BlobView {
  TIT_ASSERT(state_ == State_::executing, "Statement is not executing!");
  TIT_ASSERT(index < num_columns_(), "Column index is out of range!");
  if (column_type_(index) == SQLITE_NULL) return {};
  TIT_ASSERT(column_type_(index) == SQLITE_BLOB, "Column type mismatch!");

  const auto index_int = static_cast<int>(index);
  const auto* const value_void_ptr = sqlite3_column_blob(base(), index_int);
  if (value_void_ptr == nullptr) {
    TIT_THROW("SQLite statement failed to retrieve blob column data #{}: {}.",
              index,
              error_message(sqlite3_errcode(db_->base()), db_->base()));
  }

  const auto num_bytes_int = sqlite3_column_bytes(base(), index_int);
  if (num_bytes_int < 0) {
    TIT_THROW("SQLite statement failed to retrieve blob column size #{}: {}.",
              index,
              error_message(sqlite3_errcode(db_->base()), db_->base()));
  }

  return {static_cast<const byte_t*>(value_void_ptr),
          static_cast<size_t>(num_bytes_int)};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

BlobReader::BlobReader(const Database& db,
                       CStrView table_name,
                       CStrView column_name,
                       RowID row_id)
    : db_{&db} {
  sqlite3_blob* blob = nullptr;
  if (const auto status = sqlite3_blob_open(db.base(),
                                            "main",
                                            table_name.c_str(),
                                            column_name.c_str(),
                                            row_id,
                                            /*flags=*/0, // read-only.
                                            &blob);
      status != SQLITE_OK) {
    TIT_THROW("SQLite blob open failed ({}): {}.",
              status,
              error_message(status, db.base()));
  }

  blob_.reset(blob);
  size_ = sqlite3_blob_bytes(blob);
}

void BlobReader::Finalizer_::operator()(sqlite3_blob* blob) {
  const auto status = sqlite3_blob_close(blob);
  if (status != SQLITE_OK) {
    // Let's not throw in destructors.
    err("SQLite blob close failed ({}): {}.", status, error_message(status));
  }
}

auto BlobReader::base() const noexcept -> sqlite3_blob* {
  TIT_ASSERT(blob_.get() != nullptr, "Blob was moved away!");
  return blob_.get();
}

auto BlobReader::read(std::span<byte_t> data) -> size_t {
  TIT_ASSERT(data.size() <= std::numeric_limits<int>::max(),
             "Data size is too large!");

  TIT_ASSERT(offset_ <= size_, "Offset is out of range!");
  const auto count = std::min(data.size(), size_ - offset_);
  if (const auto status = sqlite3_blob_read(base(),
                                            data.data(),
                                            static_cast<int>(count),
                                            static_cast<int>(offset_));
      status != SQLITE_OK) {
    TIT_THROW("SQLite blob read failed ({}): {}.",
              status,
              error_message(status, db_->base()));
  }

  offset_ += count;
  return count;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

BlobWriter::BlobWriter(Database& db,
                       std::string_view table_name,
                       std::string_view column_name,
                       RowID row_id)
    : db_{&db}, //
      table_name_{table_name}, column_name_{column_name}, row_id_{row_id} {
  TIT_ASSERT(!table_name.empty(), "Table name is empty!");
  TIT_ASSERT(!column_name.empty(), "Column name is empty!");

  // Validate the table and column names to avoid SQL injection.
  constexpr auto is_valid_char = [](char c) {
    return std::isalnum(c) || c == '_';
  };
  if (!std::ranges::all_of(table_name, is_valid_char)) {
    TIT_THROW("Invalid table name: '{}'.", table_name);
  }
  if (!std::ranges::all_of(column_name, is_valid_char)) {
    TIT_THROW("Invalid column name: '{}'.", column_name);
  }
}

void BlobWriter::write(std::span<const byte_t> data) {
  buffer_.insert(buffer_.end(), data.begin(), data.end());
}

void BlobWriter::flush() {
  // Note: we cannot set table and column names as arguments, so we have to
  //       construct the SQL statement code manually.
  const auto sql = std::format("UPDATE {} SET {} = ? WHERE rowid = ?",
                               table_name_,
                               column_name_);
  Statement statement{*db_, sql};
  statement.run(buffer_, row_id_);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data::sqlite
