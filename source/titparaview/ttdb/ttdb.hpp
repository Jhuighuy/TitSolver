/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma GCC visibility push(default)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const char* ttdb__last_error();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef uint32_t ttdb_type_t;

const char* ttdb_type__kind(ttdb_type_t type);
uint32_t ttdb_type__rank(ttdb_type_t type);
uint32_t ttdb_type__dim(ttdb_type_t type);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef struct ttdb_array ttdb_array_t;

void ttdb_array__close(ttdb_array_t* array);
const char* ttdb_array__name(ttdb_array_t* array);

uint64_t ttdb_array__size(ttdb_array_t* array);
ttdb_type_t ttdb_array__type(ttdb_array_t* array);
void ttdb_array__read(ttdb_array_t* array, void* data);

typedef struct ttdb_array_iter ttdb_array_iter_t;

void ttdb_array_iter__close(ttdb_array_iter_t* iter);
ttdb_array_t* ttdb_array_iter__next(ttdb_array_iter_t* iter);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef struct ttdb_frame ttdb_frame_t;

void ttdb_frame__close(ttdb_frame_t* frame);
double ttdb_frame__time(ttdb_frame_t* frame);
uint64_t ttdb_frame__num_arrays(ttdb_frame_t* frame);
ttdb_array_t* ttdb_frame__find_array(ttdb_frame_t* frame, const char* name);
ttdb_array_iter_t* ttdb_frame__arrays(ttdb_frame_t* frame);

typedef struct ttdb_frame_iter ttdb_frame_iter_t;

void ttdb_frame_iter__close(ttdb_frame_iter_t* iter);
ttdb_frame_t* ttdb_frame_iter__next(ttdb_frame_iter_t* iter);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef struct ttdb_series ttdb_series_t;

void ttdb_series__close(ttdb_series_t* series);
uint64_t ttdb_series__num_frames(ttdb_series_t* series);

ttdb_frame_t* ttdb_series__last_frame(ttdb_series_t* series);
ttdb_frame_iter_t* ttdb_series__frames(ttdb_series_t* series);

typedef struct ttdb_series_iter ttdb_series_iter_t;

void ttdb_series_iter__close(ttdb_series_iter_t* iter);
ttdb_series_t* ttdb_series_iter__next(ttdb_series_iter_t* iter);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef struct ttdb ttdb_t;

void ttdb__close(ttdb_t* db);
ttdb_t* ttdb__open(const char* path);
uint64_t ttdb__num_series(ttdb_t* db);

ttdb_series_t* ttdb__last_series(ttdb_t* db);
ttdb_series_iter_t* ttdb__series(ttdb_t* db);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma GCC visibility pop

#ifdef __cplusplus
} // extern "C"
#endif
