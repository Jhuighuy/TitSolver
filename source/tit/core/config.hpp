/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: always_keep
#pragma once

#include <version>

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifndef TIT_DOXYGEN
#define TIT_DOXYGEN 0
#endif

#ifndef TIT_IWYU
#define TIT_IWYU 0
#endif

#ifndef TIT_GCOV
#define TIT_GCOV 0
#endif

#define TIT_NO_INLINE __attribute__((noinline))
#define TIT_FORCE_INLINE __attribute__((always_inline))

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Detect libc++
#ifdef _LIBCPP_VERSION
#define TIT_LIBCPP 1
#else
#define TIT_LIBCPP 0
#endif

#ifdef _WIN32
#define TIT_HAVE_SIGACTION 0
#else
#define TIT_HAVE_SIGACTION 1
#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifndef TIT_ENABLE_TBB
#define TIT_ENABLE_TBB 1
#endif

#ifndef TIT_ENABLE_OMP
#define TIT_ENABLE_OMP 1
#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifndef TIT_BRANCHLESS_KERNELS
#define TIT_BRANCHLESS_KERNELS 1
#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
