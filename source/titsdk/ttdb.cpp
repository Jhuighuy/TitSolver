/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>

#include "tit/core/exception.hpp"
#include "tit/core/stream.hpp"
#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"

#include "_utils.hpp"
#include "ttdb.hpp"

using namespace tit;
using namespace tit::data;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const char* ttdb_type__kind(ttdb_type_t type) {
  return safe_call([type] { return DataType{type}.kind().name(); });
}

uint32_t ttdb_type__rank(ttdb_type_t type) {
  return safe_call(
      [type] { return std::to_underlying(DataType{type}.rank()); });
}

uint32_t ttdb_type__dim(ttdb_type_t type) {
  return safe_call(
      [type] { return static_cast<uint32_t>(DataType{type}.dim()); });
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
  return safe_call([array] {
    TIT_ENSURE(array != nullptr, "Array pointer is null.");
    return array->name.c_str();
  });
}

uint64_t ttdb_array__size(ttdb_array_t* array) {
  return safe_call([array] {
    TIT_ENSURE(array != nullptr, "Array pointer is null.");
    return array->storage->array_size(array->array_id);
  });
}

ttdb_type_t ttdb_array__type(ttdb_array_t* array) {
  return safe_call([array] {
    TIT_ENSURE(array != nullptr, "Array pointer is null.");
    return array->storage->array_type(array->array_id).id();
  });
}

void ttdb_array__read(ttdb_array_t* array, void* data) {
  safe_call([array, data] {
    TIT_ENSURE(array != nullptr, "Array pointer is null.");
    TIT_ENSURE(data != nullptr, "Data pointer is null.");
    const auto type = array->storage->array_type(array->array_id);
    const auto size = array->storage->array_size(array->array_id);
    array->storage->array_data_open_read(array->array_id)
        ->read(std::span{static_cast<std::byte*>(data), size * type.width()});
    return 0;
  });
}

struct ttdb_array_iter {
  std::shared_ptr<DataStorage> storage;
  InputStreamPtr<std::pair<std::string, DataArrayID>> array_ids;
};

void ttdb_array_iter__close(ttdb_array_iter_t* iter) {
  delete iter;
}

ttdb_array_t* ttdb_array_iter__next(ttdb_array_iter_t* iter) {
  return safe_call([iter] -> ttdb_array_t* {
    TIT_ENSURE(iter != nullptr, "Array iterator pointer is null.");
    std::pair<std::string, DataArrayID> name_and_id{};
    if (iter->array_ids->read({&name_and_id, 1}) != 1) return nullptr;
    return new ttdb_array{
        .storage = iter->storage,
        .name = std::move(name_and_id.first),
        .array_id = name_and_id.second,
    };
  });
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
  return safe_call([dataset] {
    TIT_ENSURE(dataset != nullptr, "Dataset pointer is null.");
    return dataset->storage->dataset_num_arrays(dataset->dataset_id);
  });
}

ttdb_array_t* ttdb_dataset__find_array(ttdb_dataset_t* dataset,
                                       const char* name) {
  return safe_call([dataset, name] -> ttdb_array_t* {
    TIT_ENSURE(dataset != nullptr, "Dataset pointer is null.");
    TIT_ENSURE(name != nullptr, "Name pointer is null.");
    const auto id = dataset->storage->find_array_id(dataset->dataset_id, name);
    if (!id.has_value()) return nullptr;
    return new ttdb_array{
        .storage = dataset->storage,
        .name = name,
        .array_id = id.value(),
    };
  });
}

ttdb_array_iter_t* ttdb_dataset__arrays(ttdb_dataset_t* dataset) {
  return safe_call([dataset] {
    TIT_ENSURE(dataset != nullptr, "Dataset pointer is null.");
    return new ttdb_array_iter{
        .storage = dataset->storage,
        .array_ids = dataset->storage->dataset_array_ids(dataset->dataset_id),
    };
  });
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
  return safe_call([time_step] {
    TIT_ENSURE(time_step != nullptr, "Time step pointer is null.");
    return time_step->storage->time_step_time(time_step->time_step_id);
  });
}

ttdb_dataset_t* ttdb_time_step__uniforms(ttdb_time_step_t* time_step) {
  return safe_call([time_step] {
    TIT_ENSURE(time_step != nullptr, "Time step pointer is null.");
    return new ttdb_dataset{
        .storage = time_step->storage,
        .dataset_id =
            time_step->storage->time_step_uniforms_id(time_step->time_step_id),
    };
  });
}

ttdb_dataset_t* ttdb_time_step__varyings(ttdb_time_step_t* time_step) {
  return safe_call([time_step] {
    TIT_ENSURE(time_step != nullptr, "Time step pointer is null.");
    return new ttdb_dataset{
        .storage = time_step->storage,
        .dataset_id =
            time_step->storage->time_step_varyings_id(time_step->time_step_id),
    };
  });
}

struct ttdb_time_step_iter {
  std::shared_ptr<DataStorage> storage;
  InputStreamPtr<DataTimeStepID> time_step_ids;
};

void ttdb_time_step_iter__close(ttdb_time_step_iter_t* iter) {
  delete iter;
}

ttdb_time_step_t* ttdb_time_step_iter__next(ttdb_time_step_iter_t* iter) {
  return safe_call([iter] -> ttdb_time_step_t* {
    TIT_ENSURE(iter != nullptr, "Time step iterator pointer is null.");
    DataTimeStepID id{0};
    if (iter->time_step_ids->read({&id, 1}) != 1) return nullptr;
    return new ttdb_time_step{.storage = iter->storage, .time_step_id = id};
  });
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
  return safe_call([series] {
    TIT_ENSURE(series != nullptr, "Series pointer is null.");
    return series->storage->series_num_time_steps(series->series_id);
  });
}

ttdb_time_step_t* ttdb_series__last_time_step(ttdb_series_t* series) {
  return safe_call([series] {
    TIT_ENSURE(series != nullptr, "Series pointer is null.");
    return new ttdb_time_step{
        .storage = series->storage,
        .time_step_id =
            series->storage->series_last_time_step_id(series->series_id),
    };
  });
}

ttdb_time_step_iter_t* ttdb_series__time_steps(ttdb_series_t* series) {
  return safe_call([series] {
    TIT_ENSURE(series != nullptr, "Series pointer is null.");
    return new ttdb_time_step_iter{
        .storage = series->storage,
        .time_step_ids =
            series->storage->series_time_step_ids(series->series_id),
    };
  });
}

struct ttdb_series_iter {
  std::shared_ptr<DataStorage> storage;
  InputStreamPtr<DataSeriesID> series_ids;
};

void ttdb_series_iter__close(ttdb_series_iter_t* iter) {
  delete iter;
}

ttdb_series_t* ttdb_series_iter__next(ttdb_series_iter_t* iter) {
  return safe_call([iter] -> ttdb_series_t* {
    TIT_ENSURE(iter != nullptr, "Series iterator pointer is null.");
    DataSeriesID id{0};
    if (iter->series_ids->read({&id, 1}) != 1) return nullptr;
    return new ttdb_series{.storage = iter->storage, .series_id = id};
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct ttdb {
  std::shared_ptr<DataStorage> storage;
};

void ttdb__close(ttdb_t* db) {
  delete db;
}

ttdb_t* ttdb__open(const char* path) {
  return safe_call([path] {
    TIT_ENSURE(path != nullptr, "Path pointer is null.");
    return new ttdb{
        .storage = std::make_shared<DataStorage>(path, /*read_only=*/true)};
  });
}

uint64_t ttdb__num_series(ttdb_t* db) {
  return safe_call([db] {
    TIT_ENSURE(db != nullptr, "Database pointer is null.");
    return db->storage->num_series();
  });
}

ttdb_series_t* ttdb__last_series(ttdb_t* db) {
  return safe_call([db] {
    TIT_ENSURE(db != nullptr, "Database pointer is null.");
    return new ttdb_series{
        .storage = db->storage,
        .series_id = db->storage->last_series_id(),
    };
  });
}

ttdb_series_iter_t* ttdb__series(ttdb_t* db) {
  return safe_call([db] {
    TIT_ENSURE(db != nullptr, "Database pointer is null.");
    return new ttdb_series_iter{
        .storage = db->storage,
        .series_ids = db->storage->series_ids(),
    };
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
