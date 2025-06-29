/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <exception>
#include <functional>
#include <type_traits>

#include "lib.hpp"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Func>
  requires std::invocable<Func> &&
           std::default_initializable<std::invoke_result_t<Func>>
auto safe_call(Func func) noexcept -> std::invoke_result_t<Func> {
  try {
    titsdk__clear_error();
    return std::invoke(func);
  } catch (const std::exception& e) {
    titsdk__set_error(e.what());
  } catch (...) {
    titsdk__set_error("Unknown error.");
  }
  return {};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
