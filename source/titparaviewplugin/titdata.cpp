/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iterator>
#include <limits>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/utils.hpp"

#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"

#include "titdata.hpp"

using namespace tit::data;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

// Error string.
std::string error_string;

// Copy the data and release the ownership.
template<std::ranges::sized_range Range>
auto copy_and_release(Range&& range) -> std::ranges::range_value_t<Range>* {
  TIT_ASSUME_UNIVERSAL(Range, range);
  using Val = std::ranges::range_value_t<Range>;
  const auto num_bytes = sizeof(Val) * std::ranges::size(range);
  // NOLINTNEXTLINE(*-no-malloc,*-owning-memory)
  auto* const copy = static_cast<Val*>(std::malloc(num_bytes));
  if (copy == nullptr) {
    error_string =
        std::format("Failed to allocate {} bytes of memory!", num_bytes);
    return nullptr;
  }
  std::ranges::copy(range, copy);
  return copy;
}
auto copy_and_release_str(const std::string& str) -> Str_t {
  return copy_and_release(std::string_view{str.begin(), str.end() + 1});
}

// Free the data.
void free_data(void* data) {
  std::free(data); // NOLINT(*-no-malloc,*-owning-memory)
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct TitStorage {
  DataStorage s;
};

struct Darray_t {
  std::vector<tit::byte_t> d;
};

auto tit_get_error() -> const char* {
  if (error_string.empty()) return nullptr;
  return error_string.c_str();
}

void tit_free(void* ptr) {
  free_data(ptr);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tit_storage_open(const char* path) -> TitStorage* {
  const std::filesystem::path storage_path{path};
  if (!std::filesystem::exists(storage_path)) return nullptr;
  return new TitStorage{DataStorage{storage_path}}; // NOLINT(*-owning-memory)
}

#define TIT_CHECK_STORAGE(storage, ...)                                        \
  do {                                                                         \
    if ((storage) == nullptr) {                                                \
      error_string = std::format("{}: parameter `{}` must not be null!",       \
                                 TIT_STR(storage),                             \
                                 __func__);                                    \
      return __VA_ARGS__;                                                      \
    }                                                                          \
  } while (false)

void tit_storage_close(TitStorage* storage) {
  delete storage; // NOLINT(*-owning-memory)
}

auto tit_storage_path(TitStorage* storage) -> Str_t {
  TIT_CHECK_STORAGE(storage, nullptr);
  return copy_and_release_str(storage->s.path().native());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tit_storage_num_series(TitStorage* storage) -> Count_t {
  TIT_CHECK_STORAGE(storage, 0);
  return storage->s.num_series();
}

auto tit_storage_series(TitStorage* storage) -> IDs_t {
  TIT_CHECK_STORAGE(storage, {nullptr, 0});
  const auto ids = storage->s.series_ids();
  return {copy_and_release(
              std::views::transform(ids, [](auto id) { return id.get(); })),
          ids.size()};
}

auto tit_last_series(TitStorage* storage) -> ID_t {
  TIT_CHECK_STORAGE(storage, std::numeric_limits<ID_t>::max());
  return storage->s.last_series_id().get();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tit_check_series(TitStorage* storage, ID_t series_id) -> bool {
  TIT_CHECK_STORAGE(storage, false);
  return storage->s.check_series(DataSeriesID{series_id});
}

#define TIT_CHECK_SERIES(storage, series_id, ...)                              \
  do {                                                                         \
    TIT_CHECK_STORAGE(storage, __VA_ARGS__);                                   \
    if (!tit_check_series((storage), (series_id))) {                           \
      error_string = std::format(                                              \
          "{}: series specified in parameter `{}={}` does not exist!",         \
          __func__,                                                            \
          TIT_STR(series_id),                                                  \
          (series_id));                                                        \
      return __VA_ARGS__;                                                      \
    }                                                                          \
  } while (false)

auto tit_series_parameters(TitStorage* storage, ID_t series_id) -> Str_t {
  TIT_CHECK_SERIES(storage, series_id, nullptr);
  return copy_and_release_str(
      storage->s.series_parameters(DataSeriesID{series_id}));
}

auto tit_series_num_time_steps(TitStorage* storage, ID_t series_id) -> Count_t {
  TIT_CHECK_SERIES(storage, series_id, 0);
  return storage->s.series_num_time_steps(DataSeriesID{series_id});
}

auto tit_series_time_steps(TitStorage* storage, ID_t series_id) -> IDs_t {
  TIT_CHECK_SERIES(storage, series_id, {nullptr, 0});
  const auto ids = storage->s.series_time_step_ids(DataSeriesID{series_id});
  return {copy_and_release(
              std::views::transform(ids, [](auto id) { return id.get(); })),
          ids.size()};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tit_check_time_step(TitStorage* storage, ID_t time_step_id) -> bool {
  TIT_CHECK_STORAGE(storage, false);
  return storage->s.check_time_step(DataTimeStepID{time_step_id});
}

#define TIT_CHECK_TIME_STEP(storage, time_step_id, ...)                        \
  do {                                                                         \
    TIT_CHECK_STORAGE(storage, __VA_ARGS__);                                   \
    if (!tit_check_time_step((storage), (time_step_id))) {                     \
      error_string = std::format(                                              \
          "{}: time step specified in parameter `{}={}` does not exist!",      \
          __func__,                                                            \
          TIT_STR(time_step_id),                                               \
          (time_step_id));                                                     \
      return __VA_ARGS__;                                                      \
    }                                                                          \
  } while (false)

auto tit_time_step_time(TitStorage* storage, ID_t time_step_id) -> double {
  TIT_CHECK_TIME_STEP(storage,
                      time_step_id,
                      std::numeric_limits<double>::quiet_NaN());
  return storage->s.time_step_time(DataTimeStepID{time_step_id});
}

auto tit_time_step_uniforms(TitStorage* storage, ID_t time_step_id) -> ID_t {
  TIT_CHECK_TIME_STEP(storage, time_step_id, std::numeric_limits<ID_t>::max());
  return storage->s.time_step_uniforms_id(DataTimeStepID{time_step_id}).get();
}

auto tit_time_step_varyings(TitStorage* storage, ID_t time_step_id) -> ID_t {
  TIT_CHECK_TIME_STEP(storage, time_step_id, std::numeric_limits<ID_t>::max());
  return storage->s.time_step_varyings_id(DataTimeStepID{time_step_id}).get();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tit_check_set(TitStorage* storage, ID_t set_id) -> bool {
  TIT_CHECK_STORAGE(storage, false);
  return storage->s.check_dataset(DataSetID{set_id});
}

#define TIT_CHECK_SET(storage, set_id, ...)                                    \
  do {                                                                         \
    TIT_CHECK_STORAGE(storage, __VA_ARGS__);                                   \
    if (!tit_check_set((storage), (set_id))) {                                 \
      error_string = std::format(                                              \
          "{}: data set specified in parameter `{}={}` does not exist!",       \
          __func__,                                                            \
          TIT_STR(set_id),                                                     \
          (set_id));                                                           \
      return __VA_ARGS__;                                                      \
    }                                                                          \
  } while (false)

auto tit_set_num_arrays(TitStorage* storage, ID_t set_id) -> Count_t {
  TIT_CHECK_SET(storage, set_id, 0);
  return storage->s.dataset_num_arrays(DataSetID{set_id});
}

auto tit_set_arrays(TitStorage* storage, ID_t set_id) -> IDs_t {
  TIT_CHECK_SET(storage, set_id, (IDs_t{nullptr, 0}));
  const auto ids = storage->s.dataset_array_ids(DataSetID{set_id});
  return {
      copy_and_release(ids | std::views::values |
                       std::views::transform([](auto id) { return id.get(); })),
      ids.size()};
}

auto tit_set_array_names(TitStorage* storage, ID_t set_id) -> Str_t {
  TIT_CHECK_SET(storage, set_id, nullptr);
  std::string names;
  std::ranges::copy(storage->s.dataset_array_ids(DataSetID{set_id}) |
                        std::views::keys | std::views::join_with(':'),
                    std::back_inserter(names));
  return copy_and_release_str(names);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tit_check_array(TitStorage* storage, ID_t array_id) -> bool {
  return storage->s.check_array(DataArrayID{array_id});
}

#define TIT_CHECK_ARRAY(storage, array_id, ...)                                \
  do {                                                                         \
    TIT_CHECK_STORAGE(storage, __VA_ARGS__);                                   \
    if (!tit_check_array((storage), (array_id))) {                             \
      error_string = std::format(                                              \
          "{}: data array specified in parameter `{}={}` does not exist!",     \
          __func__,                                                            \
          TIT_STR(array_id),                                                   \
          (array_id));                                                         \
      return __VA_ARGS__;                                                      \
    }                                                                          \
  } while (false)

auto tit_array_dtype(TitStorage* storage, ID_t array_id) -> DtypeID_t {
  TIT_CHECK_ARRAY(storage, array_id, 0);
  return storage->s.array_type(DataArrayID{array_id}).id();
}

auto tit_array_data(TitStorage* storage, ID_t array_id) -> Darray_t* {
  TIT_CHECK_ARRAY(storage, array_id, nullptr);
  // NOLINTNEXTLINE(*-owning-memory)
  return new Darray_t{storage->s.array_data(DataArrayID{array_id})};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define TIT_CHECK_DARRAY(darray, ...)                                          \
  do {                                                                         \
    if ((darray) == nullptr) {                                                 \
      error_string = std::format("{}: parameter `{}` must not be null!",       \
                                 __func__,                                     \
                                 TIT_STR(darray));                             \
      return __VA_ARGS__;                                                      \
    }                                                                          \
  } while (false)

auto tit_darray_data(Darray_t* darray) -> void* {
  TIT_CHECK_DARRAY(darray, nullptr);
  return darray->d.data();
}

auto tit_darray_size(Darray_t* darray) -> Count_t {
  TIT_CHECK_DARRAY(darray, 0);
  return darray->d.size();
}

void tit_darray_close(Darray_t* darray) {
  delete darray; // NOLINT(*-owning-memory)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tit_check_dtype(DtypeID_t dtype) -> bool {
  return DataType{dtype}.known();
}

#define TIT_CHECK_DTYPE(dtype, ...)                                            \
  do {                                                                         \
    if (!tit_check_dtype((dtype))) {                                           \
      error_string = std::format(                                              \
          "{}: data type specified in parameter `{}={}` is not known!",        \
          __func__,                                                            \
          TIT_STR(dtype),                                                      \
          (dtype));                                                            \
      return __VA_ARGS__;                                                      \
    }                                                                          \
  } while (false)

auto tit_dtype_kind(DtypeID_t dtype) -> const char* {
  TIT_CHECK_DTYPE(dtype, nullptr);
  const auto kind = DataType{dtype}.kind();
  switch (kind) {
    case DataType::Kind::int8:    return "int8";
    case DataType::Kind::uint8:   return "uint8";
    case DataType::Kind::int16:   return "int16";
    case DataType::Kind::uint16:  return "uint16";
    case DataType::Kind::int32:   return "int32";
    case DataType::Kind::uint32:  return "uint32";
    case DataType::Kind::int64:   return "int64";
    case DataType::Kind::uint64:  return "uint64";
    case DataType::Kind::float32: return "float32";
    case DataType::Kind::float64: return "float64";
    case DataType::Kind::unknown:
    case DataType::Kind::count_:
    default:                      {
      error_string = std::format(
          "tit_dtype_kind: internal error, invalid data type kind {}!",
          static_cast<int>(std::to_underlying(kind)));
      return nullptr;
    }
  }
}

auto tit_dtype_dim(DtypeID_t dtype) -> Count_t {
  TIT_CHECK_DTYPE(dtype, 0);
  return DataType{dtype}.dim();
}

auto tit_dtype_rank(DtypeID_t dtype) -> Count_t {
  TIT_CHECK_DTYPE(dtype, 0);
  return std::to_underlying(DataType{dtype}.rank());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
