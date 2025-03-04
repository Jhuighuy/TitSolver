/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// @todo Tests are missing for the entire plugin code.
/// @todo Currently, there is no way to pass the errors from the C++ code to
///       to the user, the whole application will crash if an error occurs.
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/stream.hpp"

#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"

#include "ttdb.hpp"

// NOLINTBEGIN(modernize-*,*-owning-memory)

using namespace tit;
using namespace tit::data;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const char* ttdb_type__kind(ttdb_type_t type) {
  return DataType{type}.kind().name().c_str();
}

uint32_t ttdb_type__rank(ttdb_type_t type) {
  return std::to_underlying(DataType{type}.rank());
}

uint32_t ttdb_type__dim(ttdb_type_t type) {
  return static_cast<uint32_t>(DataType{type}.dim());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct ttdb_array {
  std::shared_ptr<DataStorage> storage;
  std::string name;
  DataArrayID array_id;
};

void ttdb_array__close(ttdb_array_t* array) {
  delete array;
}

const char* ttdb_array__name(ttdb_array_t* array) {
  return array->name.c_str();
}

uint64_t ttdb_array__size(ttdb_array_t* array) {
  return array->storage->array_size(array->array_id);
}

ttdb_type_t ttdb_array__type(ttdb_array_t* array) {
  return array->storage->array_type(array->array_id).id();
}

void ttdb_array__read(ttdb_array_t* array, void* data) {
  const auto type = array->storage->array_type(array->array_id);
  const auto size = array->storage->array_size(array->array_id);
  array->storage->array_data_open_read(array->array_id)
      ->read(std::span{static_cast<byte_t*>(data), size * type.width()});
}

struct ttdb_array_iter {
  std::shared_ptr<DataStorage> storage;
  InputStreamPtr<std::pair<std::string, DataArrayID>> array_ids;
};

void ttdb_array_iter__close(ttdb_array_iter_t* iter) {
  delete iter;
}

ttdb_array_t* ttdb_array_iter__next(ttdb_array_iter_t* iter) {
  std::pair<std::string, DataArrayID> name_and_id{std::string{}, 0};
  if (iter->array_ids->read({&name_and_id, 1}) != 1) return nullptr;
  return new ttdb_array{
      .storage = iter->storage,
      .name = std::move(name_and_id.first),
      .array_id = name_and_id.second,
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct ttdb_dataset {
  std::shared_ptr<DataStorage> storage;
  DataSetID dataset_id;
};

void ttdb_dataset__close(ttdb_dataset_t* dataset) {
  delete dataset;
}

uint64_t ttdb_dataset__num_arrays(ttdb_dataset_t* dataset) {
  return dataset->storage->dataset_num_arrays(dataset->dataset_id);
}

ttdb_array_t* ttdb_dataset__find_array(ttdb_dataset_t* dataset,
                                       const char* name) {
  const auto id = dataset->storage->find_array_id(dataset->dataset_id, name);
  if (!id.has_value()) return nullptr;
  return new ttdb_array{
      .storage = dataset->storage,
      .name = name,
      .array_id = id.value(),
  };
}

ttdb_array_iter_t* ttdb_dataset__arrays(ttdb_dataset_t* dataset) {
  return new ttdb_array_iter{
      .storage = dataset->storage,
      .array_ids = dataset->storage->dataset_array_ids(dataset->dataset_id),
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct ttdb_time_step {
  std::shared_ptr<DataStorage> storage;
  DataTimeStepID time_step_id;
};

void ttdb_time_step__close(ttdb_time_step_t* time_step) {
  delete time_step;
}

double ttdb_time_step__time(ttdb_time_step_t* time_step) {
  return time_step->storage->time_step_time(time_step->time_step_id);
}

ttdb_dataset_t* ttdb_time_step__uniforms(ttdb_time_step_t* time_step) {
  return new ttdb_dataset{
      .storage = time_step->storage,
      .dataset_id =
          time_step->storage->time_step_uniforms_id(time_step->time_step_id),
  };
}

ttdb_dataset_t* ttdb_time_step__varyings(ttdb_time_step_t* time_step) {
  return new ttdb_dataset{
      .storage = time_step->storage,
      .dataset_id =
          time_step->storage->time_step_varyings_id(time_step->time_step_id),
  };
}

struct ttdb_time_step_iter {
  std::shared_ptr<DataStorage> storage;
  InputStreamPtr<DataTimeStepID> time_step_ids;
};

void ttdb_time_step_iter__close(ttdb_time_step_iter_t* iter) {
  delete iter;
}

ttdb_time_step_t* ttdb_time_step_iter__next(ttdb_time_step_iter_t* iter) {
  DataTimeStepID id{0};
  if (iter->time_step_ids->read({&id, 1}) != 1) return nullptr;
  return new ttdb_time_step{.storage = iter->storage, .time_step_id = id};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct ttdb_series {
  std::shared_ptr<DataStorage> storage;
  DataSeriesID series_id;
};

void ttdb_series__close(ttdb_series_t* series) {
  delete series;
}

uint64_t ttdb_series__num_time_steps(ttdb_series_t* series) {
  return series->storage->series_num_time_steps(series->series_id);
}

ttdb_time_step_t* ttdb_series__last_time_step(ttdb_series_t* series) {
  return new ttdb_time_step{
      .storage = series->storage,
      .time_step_id =
          series->storage->series_last_time_step_id(series->series_id),
  };
}

ttdb_time_step_iter_t* ttdb_series__time_steps(ttdb_series_t* series) {
  return new ttdb_time_step_iter{
      .storage = series->storage,
      .time_step_ids = series->storage->series_time_step_ids(series->series_id),
  };
}

struct ttdb_series_iter {
  std::shared_ptr<DataStorage> storage;
  InputStreamPtr<DataSeriesID> series_ids;
};

void ttdb_series_iter__close(ttdb_series_iter_t* iter) {
  delete iter;
}

ttdb_series_t* ttdb_series_iter__next(ttdb_series_iter_t* iter) {
  DataSeriesID id{0};
  if (iter->series_ids->read({&id, 1}) != 1) return nullptr;
  return new ttdb_series{.storage = iter->storage, .series_id = id};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct ttdb {
  std::shared_ptr<DataStorage> storage;
};

void ttdb__close(ttdb_t* db) {
  delete db;
}

ttdb_t* ttdb__open(const char* path) {
  return new ttdb{.storage = std::make_shared<DataStorage>(path)};
}

uint64_t ttdb__num_series(ttdb_t* db) {
  return db->storage->num_series();
}

ttdb_series_t* ttdb__last_series(ttdb_t* db) {
  return new ttdb_series{
      .storage = db->storage,
      .series_id = db->storage->last_series_id(),
  };
}

ttdb_series_iter_t* ttdb__series(ttdb_t* db) {
  return new ttdb_series_iter{
      .storage = db->storage,
      .series_ids = db->storage->series_ids(),
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(modernize-*,*-owning-memory)
