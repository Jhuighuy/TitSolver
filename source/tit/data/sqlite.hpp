/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <filesystem>
#include <memory>
#include <ranges>
#include <string_view>
#include <tuple>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/utils.hpp"

struct sqlite3;
struct sqlite3_stmt;

namespace tit::data::sqlite {

/// SQLite row ID type.
using RowID = int64_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// SQLite database.
class Database final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Open or create a database file.
  explicit Database(const std::filesystem::path& path);

  /// SQLite database object.
  auto base() const noexcept -> sqlite3*;

  /// Database path, empty if in-memory.
  auto path() const -> std::filesystem::path;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Execute a SQL statement.
  void execute(const char* sql) const;

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

/// Text argument type.
template<class Text>
concept text_arg = std::constructible_from<std::string_view, Text>;

/// Blob argument type.
template<class Blob>
concept blob_arg = std::constructible_from<ByteSpan, Blob>;

/// Statement argument type.
template<class Value>
concept arg = std::integral<Value> || std::floating_point<Value> ||
              text_arg<Value> || blob_arg<Value>;

/// Text column type.
template<class Text>
concept text_column =
    std::is_object_v<Text> && std::constructible_from<Text, std::string_view>;

/// Blob column type.
template<class Blob>
concept blob_column =
    std::is_object_v<Blob> &&
    (std::constructible_from<Blob, ByteSpan> || requires(ByteSpan b) {
      { b | std::ranges::to<Blob>() } -> std::same_as<Blob>;
    });

/// Column type.
template<class Value>
concept column = std::integral<Value> || std::floating_point<Value> ||
                 text_column<Value> || blob_column<Value>;

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
    size_t index = 0;
    (..., [&index, this]<class Arg>(const Arg& arg) {
      index += 1; // Statement arguments are one-based.
      if constexpr (std::integral<Arg>) {
        bind_(index, static_cast<int64_t>(arg));
      } else if constexpr (std::floating_point<Arg>) {
        bind_(index, static_cast<float64_t>(arg));
      } else if constexpr (text_arg<Arg> || blob_arg<Arg>) {
        bind_(index, arg);
      } else static_assert(false);
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
    size_t index = npos;
    return {
      [&index, this]<class Column>() {
        index += 1; // Columns are zero-based.
        if constexpr (std::integral<Column>) {
          return static_cast<Column>(column_int_(index));
        } else if constexpr (std::floating_point<Column>) {
          return static_cast<Column>(column_real_(index));
        } else if constexpr (text_column<Column>) {
          return Column{column_text_(index)};
        } else if constexpr (blob_column<Column>) {
          const auto blob = column_blob_(index);
          if constexpr (std::constructible_from<Column, ByteSpan>) return blob;
          else return blob | std::ranges::to<Column>();
        } else static_assert(false);
      }.template operator()<Columns>()...
    };
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
  void bind_(size_t index, ByteSpan value) const;

  // Get the statement columns.
  auto num_columns_() const -> size_t;
  auto column_type_(size_t index) const -> int;
  auto column_int_(size_t index) const -> int64_t;
  auto column_real_(size_t index) const -> float64_t;
  auto column_text_(size_t index) const -> std::string_view;
  auto column_blob_(size_t index) const -> ByteSpan;

  Database* db_;
  std::unique_ptr<sqlite3_stmt, Finalizer_> stmt_;
  State_ state_ = State_::invalid;

}; // class Statement

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data::sqlite
