/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

// NOLINTBEGIN(*-reserved-identifier,cert-*,modernize-*)

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

typedef struct ttdb_dataset ttdb_dataset_t;

void ttdb_dataset__close(ttdb_dataset_t* dataset);
uint64_t ttdb_dataset__num_arrays(ttdb_dataset_t* dataset);
ttdb_array_t* ttdb_dataset__find_array(ttdb_dataset_t* dataset,
                                       const char* name);
ttdb_array_iter_t* ttdb_dataset__arrays(ttdb_dataset_t* dataset);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef struct ttdb_time_step ttdb_time_step_t;

void ttdb_time_step__close(ttdb_time_step_t* time_step);
double ttdb_time_step__time(ttdb_time_step_t* time_step);
ttdb_dataset_t* ttdb_time_step__uniforms(ttdb_time_step_t* time_step);
ttdb_dataset_t* ttdb_time_step__varyings(ttdb_time_step_t* time_step);

typedef struct ttdb_time_step_iter ttdb_time_step_iter_t;

void ttdb_time_step_iter__close(ttdb_time_step_iter_t* iter);
ttdb_time_step_t* ttdb_time_step_iter__next(ttdb_time_step_iter_t* iter);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef struct ttdb_series ttdb_series_t;

void ttdb_series__close(ttdb_series_t* series);
uint64_t ttdb_series__num_time_steps(ttdb_series_t* series);
ttdb_time_step_t* ttdb_series__last_time_step(ttdb_series_t* series);
ttdb_time_step_iter_t* ttdb_series__time_steps(ttdb_series_t* series);

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

// NOLINTEND(*-reserved-identifier,cert-*,modernize-*)
