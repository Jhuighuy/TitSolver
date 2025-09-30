/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/str.hpp"
#include "tit/core/stream.hpp"

struct sqlite3;
struct sqlite3_stmt;
struct sqlite3_blob;

namespace tit::data::sqlite {

/// SQLite row ID type.
using RowID = int64_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// SQLite database.
class Database final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Open or create a database file.
  explicit Database(const std::filesystem::path& path, bool read_only = false);

  /// SQLite database object.
  auto base() const noexcept -> sqlite3*;

  /// Database path, empty if in-memory.
  auto path() const -> std::filesystem::path;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Execute a SQL statement.
  void execute(const std::string& sql) const;

  /// Get the last insert row ID.
  auto last_insert_row_id() const -> RowID;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  struct Closer_ final {
    static void operator()(sqlite3* db);
  };

  std::unique_ptr<sqlite3, Closer_> db_;

}; // class Database

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Blob view type.
using BlobView = std::span<const std::byte>;

/// Blob argument type.
template<class Blob>
concept blob_arg = std::constructible_from<BlobView, Blob>;

/// Statement argument type.
template<class Value>
concept arg = std::integral<Value> || std::is_enum_v<Value> ||
              std::floating_point<Value> || str_like<Value> || blob_arg<Value>;

/// Blob column type.
template<class Blob>
concept blob_column =
    std::is_object_v<Blob> && std::constructible_from<BlobView, Blob>;

/// Column type.
template<class Value>
concept column =
    std::integral<Value> || std::is_enum_v<Value> ||
    std::floating_point<Value> || str_like<Value> || blob_column<Value>;

/// SQLite statement.
class Statement final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Execute a SQL statement.
  explicit Statement(Database& db, std::string_view sql);

  /// SQLite statement object.
  auto base() const noexcept -> sqlite3_stmt*;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Bind the statement arguments.
  template<arg... Args>
    requires (sizeof...(Args) > 0)
  void bind(const Args&... args) {
    TIT_ASSERT(sizeof...(Args) == num_params_(),
               "Number of arguments does not match the number of parameters!");
    if (state_ == State_::finished) reset();
    auto index = std::numeric_limits<size_t>::max();
    (..., [&index, this]<class Arg>(const Arg& arg) {
      index += 1;
      if constexpr (std::integral<Arg> || std::is_enum_v<Arg>) {
        bind_(index, static_cast<int64_t>(arg));
      } else if constexpr (std::floating_point<Arg>) {
        bind_(index, static_cast<float64_t>(arg));
      } else if constexpr (str_like<Arg> || blob_arg<Arg>) {
        bind_(index, arg);
      } else {
        static_assert(false);
      }
    }(args));
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Step the statement.
  ///
  /// Stament must be prepared or in the executing state.
  ///
  /// @returns `true` if the statement has more rows to return, `false` if it
  ///          has no more rows to return and the iteration is finished.
  [[nodiscard]] auto step() -> bool;

  /// Reset the statement execution. No binds are reset.
  ///
  /// Statement must be finished.
  void reset();

  /// Run the statement, assuming all arguments are bound.
  ///
  /// Statement must finish in a single step. For multi-step statements, use
  /// `step` and `reset` instead.
  ///
  /// Statement must be either prepared or finished. In the latter case, the
  /// statement is reset.
  void run();

  /// Bind the statement arguments and run it, and reset the statement.
  template<arg... Args>
    requires (sizeof...(Args) > 0)
  void run(const Args&... args) {
    bind(args...);
    run();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Get the column values from the current row.
  ///
  /// Statement must be executing.
  ///
  /// @returns Value of the column.
  /// @note Text or blob columns can return a view of the underlying data,
  ///       but the returned view is only valid until the next call to `step`.
  template<column... Columns>
    requires (sizeof...(Columns) > 0)
  auto columns() const -> std::tuple<Columns...> {
    TIT_ASSERT(sizeof...(Columns) == num_columns_(),
               "Number of return values does not match the number of columns!");
    size_t index = std::numeric_limits<size_t>::max();
    return {[&index, this]<class Column>() {
      index += 1;
      if constexpr (std::integral<Column> || std::is_enum_v<Column>) {
        return static_cast<Column>(column_int_(index));
      } else if constexpr (std::floating_point<Column>) {
        return static_cast<Column>(column_real_(index));
      } else if constexpr (str_like<Column>) {
        return Column{column_text_(index)};
      } else if constexpr (blob_column<Column>) {
        const auto blob = column_blob_(index);
        if constexpr (std::same_as<Column, BlobView>) return blob;
        else return blob | std::ranges::to<Column>();
      } else {
        static_assert(false);
      }
    }.template operator()<Columns>()...};
  }

  /// Shorthand for a single column query.
  template<column Column>
  auto column() const -> Column {
    return std::get<0>(columns<Column>());
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  enum class State_ : uint8_t {
    invalid,
    prepared,
    executing,
    finished,
  };

  struct Finalizer_ final {
    static void operator()(sqlite3_stmt* stmt);
  };

  // Bind the statement arguments.
  auto num_params_() const -> size_t;
  void bind_(size_t index, int64_t value) const;
  void bind_(size_t index, float64_t value) const;
  void bind_(size_t index, std::string_view value) const;
  void bind_(size_t index, BlobView value) const;

  // Get the statement columns.
  auto num_columns_() const -> size_t;
  auto column_type_(size_t index) const -> int;
  auto column_int_(size_t index) const -> int64_t;
  auto column_real_(size_t index) const -> float64_t;
  auto column_text_(size_t index) const -> std::string_view;
  auto column_blob_(size_t index) const -> BlobView;

  Database* db_;
  std::unique_ptr<sqlite3_stmt, Finalizer_> stmt_;
  State_ state_ = State_::invalid;

}; // class Statement

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// SQLite blob reader.
class BlobReader final : public InputStream<std::byte> {
public:

  /// Open a blob from a database.
  BlobReader(const Database& db,
             const std::string& table_name,
             const std::string& column_name,
             RowID row_id);

  /// SQLite blob object.
  auto base() const noexcept -> sqlite3_blob*;

  /// Read the next bytes from the blob.
  auto read(std::span<std::byte> data) -> size_t override;

private:

  struct Finalizer_ final {
    static void operator()(sqlite3_blob* blob);
  };

  const Database* db_;
  std::unique_ptr<sqlite3_blob, Finalizer_> blob_;
  size_t size_ = 0;
  size_t offset_ = 0;

}; // class BlobReader

/// Make a blob reader.
constexpr auto make_blob_reader(const Database& db,
                                const std::string& table_name,
                                const std::string& column_name,
                                RowID row_id) -> InputStreamPtr<std::byte> {
  return std::make_unique<BlobReader>(db, table_name, column_name, row_id);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// SQLite blob writer.
class BlobWriter final : public OutputStream<std::byte> {
public:

  /// Open a blob in a database.
  BlobWriter(Database& db,
             std::string_view table_name,
             std::string_view column_name,
             RowID row_id);

  /// Write the next bytes to the blob.
  void write(std::span<const std::byte> data) override;

  /// Flush the stream.
  void flush() override;

private:

  Database* db_;
  std::string table_name_;
  std::string column_name_;
  RowID row_id_;
  std::vector<std::byte> buffer_;

}; // class BlobWriter

/// Make a blob writer.
constexpr auto make_blob_writer(Database& db,
                                std::string_view table_name,
                                std::string_view column_name,
                                RowID row_id) -> OutputStreamPtr<std::byte> {
  return make_flushable<BlobWriter>(db, table_name, column_name, row_id);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data::sqlite
