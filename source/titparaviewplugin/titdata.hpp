/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstdint>

extern "C" {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Count type. Always 64-bit.
using Count_t = uint64_t;

// String type. Has to be freed with `tit_free`.
using Str_t = char*;

// String view type. Does not have to be freed.
using StrView_t = const char*;

// ID type. Always 64-bit.
using ID_t = int64_t;

// ID pointer type.
struct IDs_t {
  ID_t* ids;    // IDs. Has to be freed with `tit_free`.
  Count_t size; // Number of IDs.
};

// Data type ID type. Always 32-bit.
using DtypeID_t = uint32_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Get description of the last error, null if no error.
// Pointer return by this function must *NOT* be freed with `tit_free`.
auto tit_get_error() -> StrView_t;

// Free memory allocated by the functions in this header.
void tit_free(void* ptr);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Data storage opaque handle.
struct TitStorage;

// Open a storage.
// Result must be closed with `tit_storage_close`.
auto tit_storage_open(const char* path) -> TitStorage*;

// Close a storage.
void tit_storage_close(TitStorage* storage);

// Get the storage path.
// Pointer return by this function must be freed with `tit_free`.
auto tit_storage_path(TitStorage* storage) -> Str_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Get the number of data series.
auto tit_storage_num_series(TitStorage* storage) -> Count_t;

// Enumerate data series.
auto tit_storage_series(TitStorage* storage) -> IDs_t;

// Get the last data series.
auto tit_last_series(TitStorage* storage) -> ID_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Check if a data series exists.
auto tit_check_series(TitStorage* storage, ID_t series_id) -> bool;

// Get the parameters of a data series.
// Pointer return by this function must be freed with `tit_free`.
auto tit_series_parameters(TitStorage* storage, ID_t series_id) -> Str_t;

// Get the number of time steps in a data series.
auto tit_series_num_time_steps(TitStorage* storage, ID_t series_id) -> Count_t;

// Enumerate time step IDs in a data series.
// Pointer return by this function must be freed with `tit_free`.
auto tit_series_time_steps(TitStorage* storage, ID_t series_id) -> IDs_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Check if a time step exists.
auto tit_check_time_step(TitStorage* storage, ID_t time_step_id) -> bool;

// Get the time of a time step.
auto tit_time_step_time(TitStorage* storage, ID_t time_step_id) -> double;

// Get the uniform data set ID of a time step.
auto tit_time_step_uniforms(TitStorage* storage, ID_t time_step_id) -> ID_t;

// Get the varying data set ID of a time step.
auto tit_time_step_varyings(TitStorage* storage, ID_t time_step_id) -> ID_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Check if a data set exists.
auto tit_check_set(TitStorage* storage, ID_t set_id) -> bool;

// Get the number of data arrays in a data set.
auto tit_set_num_arrays(TitStorage* storage, ID_t set_id) -> Count_t;

// Enumerate data array IDs in a data set.
// Pointer return by this function must be freed with `tit_free`.
auto tit_set_arrays(TitStorage* storage, ID_t set_id) -> IDs_t;

// Get a colon-separated list of data array names in a data set.
// Pointer return by this function must be freed with `tit_free`.
auto tit_set_array_names(TitStorage* storage, ID_t set_id) -> Str_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Check if a data array exists.
auto tit_check_array(TitStorage* storage, ID_t array_id) -> bool;

// Get the data type of a data array.
auto tit_array_dtype(TitStorage* storage, ID_t array_id) -> DtypeID_t;

// Read all the data from the storage into a `Darray_t`.
// Result must be closed with `tit_darray_close`.
auto tit_array_data(TitStorage* storage, ID_t array_id) -> struct Darray_t*;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Data array opaque handle.
struct Darray_t;

// Get pointer to the data of a data array.
// Pointer return by this function must *NOT* be freed.
auto tit_darray_data(Darray_t* darray) -> void*;

// Get size of the data of a data array (in bytes).
auto tit_darray_size(Darray_t* darray) -> Count_t;

// Close a data array.
void tit_darray_close(Darray_t* darray);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Check if data type is known.
auto tit_check_dtype(DtypeID_t dtype) -> bool;

// Get kind of a data type.
// Pointer return by this function must *NOT* be freed.
auto tit_dtype_kind(DtypeID_t dtype) -> StrView_t;

// Get dimension of a data type.
auto tit_dtype_dim(DtypeID_t dtype) -> Count_t;

// Get rank of a data type.
auto tit_dtype_rank(DtypeID_t dtype) -> Count_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // extern "C"
