/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

namespace tit::app {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** `main`-like function pointer. */
using main_like_t = int (*)(int, char**);

/** Wrapper for the main function that should run sets up the
 ** environment: initialized threading, error handlers, etc.
 **
 ** Example:
 ** @code
 ** int main(int argc, char** argv) {
 **   return tit::wrap_main(argc, argv, [](int the_argc, char** the_argv) {
 **     ...
 **   });
 ** }
 ** @endcode */
auto wrap_main(int argc, char** argv, main_like_t main_func) noexcept -> int;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::app
