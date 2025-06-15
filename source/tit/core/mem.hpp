/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// @todo Tests are missing.
#pragma once

#include <cstdlib>
#include <memory>

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Unique pointer to an array.
/// This alias exists because clang-tidy triggers "avoid C-style arrays"
/// diagnostics for `std::unique_ptr<T[]>`.
// NOLINTBEGIN(*-c-arrays)
template<class T, class Deleter = std::default_delete<T[]>>
using UniqueArrayPtr = std::unique_ptr<T[], Deleter>;
// NOLINTEND(*-c-arrays)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Deleter for `std::unique_ptr` that calls `std::free`.
struct FreeFunc final {
  static void operator()(void* ptr) noexcept {
    std::free(ptr); // NOLINT(*-owning-memory,*-no-malloc)
  }
};

/// Unique pointer to `malloc`/`realloc`-allocated object.
template<class T>
using MallocPtr = std::unique_ptr<T, FreeFunc>;

/// Unique pointer to `malloc`/`realloc`-allocated array.
template<class T>
using MallocArrayPtr = UniqueArrayPtr<T, FreeFunc>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
