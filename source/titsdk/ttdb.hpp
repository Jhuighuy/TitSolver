/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @addtogroup titsdk
 * @{
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma GCC visibility push(default)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Type of data in an array.
 */
typedef uint32_t ttdb_type_t;

/**
 * Get the string representation of the kind of a type.
 *
 * @param type Type of data.
 * @returns Pointer to a null-terminated string containing kind of the type.
 *          The string points to an internal buffer, so it should not be freed.
 */
const char* ttdb_type__kind(ttdb_type_t type);

/**
 * Get the rank of a type.
 *
 * @param type Type of data.
 * @returns Rank of the type.
 */
uint32_t ttdb_type__rank(ttdb_type_t type);

/**
 * Get the dimension of a type.
 *
 * @param type Type of data.
 * @returns Dimension of the type.
 */
uint32_t ttdb_type__dim(ttdb_type_t type);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Array of data.
 */
typedef struct ttdb_array ttdb_array_t;

/**
 * Close the array.
 *
 * @param array Array to close.
 */
void ttdb_array__close(ttdb_array_t* array);

/**
 * Get the name of the array.
 *
 * @param array Array to get the name of.
 * @returns Pointer to a null-terminated string containing the name of the
 *          array. The string points to an internal buffer, so it should not be
 *          freed.
 */
const char* ttdb_array__name(ttdb_array_t* array);

/**
 * Get the size of the array, in number of elements.
 *
 * @param array Array to get the size of.
 * @returns Size of the array.
 */
uint64_t ttdb_array__size(ttdb_array_t* array);

/**
 * Get the type of the array.
 *
 * @param array Array to get the type of.
 * @returns Type of the array.
 */
ttdb_type_t ttdb_array__type(ttdb_array_t* array);

/**
 * Read the array data into the provided buffer.
 *
 * @param array Array to read.
 * @param data Pointer to the buffer to read the data into.
 */
void ttdb_array__read(ttdb_array_t* array, void* data);

/**
 * Iterator over arrays in a dataset.
 */
typedef struct ttdb_array_iter ttdb_array_iter_t;

/**
 * Close the iterator.
 *
 * @param iter Iterator to close.
 */
void ttdb_array_iter__close(ttdb_array_iter_t* iter);

/**
 * Get the current array from the iterator and advance the iterator.
 *
 * @param iter Iterator to get the current array from.
 * @returns Pointer to the current array from the iterator.
 *          If there are no more arrays, returns null.
 */
ttdb_array_t* ttdb_array_iter__next(ttdb_array_iter_t* iter);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Frame containing arrays of data.
 */
typedef struct ttdb_frame ttdb_frame_t;

/**
 * Close the frame.
 *
 * @param frame Frame to close.
 */
void ttdb_frame__close(ttdb_frame_t* frame);

/**
 * Get the time of the frame.
 *
 * @param frame Frame to get the time of.
 * @returns Time of the frame.
 */
double ttdb_frame__time(ttdb_frame_t* frame);

/**
 * Get the number of arrays in the frame.
 *
 * @param frame Frame to get the number of arrays of.
 * @returns Number of arrays in the frame.
 */
uint64_t ttdb_frame__num_arrays(ttdb_frame_t* frame);

/**
 * Find an array by name in the frame.
 *
 * @param frame Frame to find the array in.
 * @param name Name of the array to find.
 * @returns Pointer to the array with the given name. If the array does not
 *          exist, returns null.
 */
ttdb_array_t* ttdb_frame__find_array(ttdb_frame_t* frame, const char* name);

/**
 * Iterate over all arrays in the frame.
 *
 * @param frame Frame to iterate over.
 * @returns Iterator over all arrays in the frame.
 */
ttdb_array_iter_t* ttdb_frame__arrays(ttdb_frame_t* frame);

/**
 * Iterator over frames in a series.
 */
typedef struct ttdb_frame_iter ttdb_frame_iter_t;

/**
 * Close the iterator.
 *
 * @param iter Iterator to close.
 */
void ttdb_frame_iter__close(ttdb_frame_iter_t* iter);

/**
 * Get the current frame from the iterator and advance the iterator.
 *
 * @param iter Iterator to get the current frame from.
 * @returns Pointer to the current frame from the iterator.
 *          If there are no more frames, returns null.
 */
ttdb_frame_t* ttdb_frame_iter__next(ttdb_frame_iter_t* iter);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Series in a storage.
 */
typedef struct ttdb_series ttdb_series_t;

/**
 * Close the series.
 *
 * @param series Series to close.
 */
void ttdb_series__close(ttdb_series_t* series);

/**
 * Get the number of frames in the series.
 *
 * @param series Series to get the number of frames of.
 * @returns Number of frames in the series.
 */
uint64_t ttdb_series__num_frames(ttdb_series_t* series);

/**
 * Get the last frame in the series.
 *
 * @param series Series to get the last frame of.
 * @returns Pointer to the last frame in the series.
 */
ttdb_frame_t* ttdb_series__last_frame(ttdb_series_t* series);

/**
 * Iterate over all frames in the series.
 *
 * @param series Series to iterate over.
 * @returns Iterator over all frames in the series.
 */
ttdb_frame_iter_t* ttdb_series__frames(ttdb_series_t* series);

/**
 * Iterator over series in a storage.
 */
typedef struct ttdb_series_iter ttdb_series_iter_t;

/**
 * Close the iterator.
 *
 * @param iter Iterator to close.
 */
void ttdb_series_iter__close(ttdb_series_iter_t* iter);

/**
 * Get the current series from the iterator and advance the iterator.
 *
 * @param iter Iterator to get the current series from.
 * @returns Pointer to the current series from the iterator.
 *          If there are no more series, returns null.
 */
ttdb_series_t* ttdb_series_iter__next(ttdb_series_iter_t* iter);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * BlueTit particle storage.
 */
typedef struct ttdb ttdb_t;

/**
 * Close the storage.
 *
 * @param db Storage to close.
 */
void ttdb__close(ttdb_t* db);

/**
 * Open the storage at the given path.
 *
 * @param path Path to the storage.
 * @returns Pointer to the storage.
 */
ttdb_t* ttdb__open(const char* path);

/**
 * Get the number of series in the storage.
 *
 * @param db Storage to get the number of series of.
 * @returns Number of series in the storage.
 */
uint64_t ttdb__num_series(ttdb_t* db);

/**
 * Get the last series in the storage.
 *
 * @param db Storage to get the last series of.
 * @returns Pointer to the last series in the storage.
 */
ttdb_series_t* ttdb__last_series(ttdb_t* db);

/**
 * Iterate over all series in the storage.
 *
 * @param db Storage to iterate over.
 * @returns Iterator over all series in the storage.
 */
ttdb_series_iter_t* ttdb__series(ttdb_t* db);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma GCC visibility pop

#ifdef __cplusplus
} // extern "C"
#endif

/**
 * @}
 */
