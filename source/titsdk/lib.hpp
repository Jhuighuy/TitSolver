/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @defgroup titsdk BlueTit SDK C API.
 * @addtogroup titsdk
 * @{
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#pragma GCC visibility push(default)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Get the last error message.
 *
 * @returns Pointer to a null-terminated string containing the last error
 *          message. If no error has occurred, returns null.
 */
const char* titsdk__last_error();

/**
 * Clear the last error message.
 */
void titsdk__clear_error();

/**
 * Set the last error message.
 *
 * @param error Pointer to a null-terminated string containing the error
 *              message. This string will be copied internally.
 */
void titsdk__set_error(const char* error);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma GCC visibility pop

#ifdef __cplusplus
} // extern "C"
#endif

/**
 * @}
 */
