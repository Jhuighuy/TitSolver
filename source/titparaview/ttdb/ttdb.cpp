/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

#include "tit/core/exception.hpp"
#include "tit/core/stream.hpp"
#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"

#include "ttdb.hpp"

using namespace tit;
using namespace tit::data;

using StoragePtr = std::shared_ptr<Storage>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

std::array<char, 1024> last_error;

template<class Func>
  requires std::invocable<Func> &&
           std::default_initializable<std::invoke_result_t<Func>>
auto safe_call(Func func) noexcept -> std::invoke_result_t<Func> {
  try {
    last_error.front() = '\0';
    return std::invoke(func);
  } catch (const std::exception& e) {
    std::strncpy(last_error.data(), e.what(), last_error.size() - 1);
  } catch (...) {
    std::strncpy(last_error.data(), "Unknown error.", last_error.size() - 1);
  }
  return {};
}

} // namespace

const char* ttdb__last_error() {
  return last_error.front() == '\0' ? nullptr : last_error.data();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const char* ttdb_type__kind(ttdb_type_t type) {
  return safe_call([type] { return Type{type}.kind().name(); });
}

uint32_t ttdb_type__rank(ttdb_type_t type) {
  return safe_call([type] { return std::to_underlying(Type{type}.rank()); });
}

uint32_t ttdb_type__dim(ttdb_type_t type) {
  return safe_call([type] { return static_cast<uint32_t>(Type{type}.dim()); });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct ttdb_array {
  StoragePtr storage;
  ArrayID id;
  std::string name;
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
    return array->storage->array_size(array->id);
  });
}

ttdb_type_t ttdb_array__type(ttdb_array_t* array) {
  return safe_call([array] {
    TIT_ENSURE(array != nullptr, "Array pointer is null.");
    return array->storage->array_type(array->id).id();
  });
}

void ttdb_array__read(ttdb_array_t* array, void* data) {
  safe_call([array, data] {
    TIT_ENSURE(array != nullptr, "Array pointer is null.");
    TIT_ENSURE(data != nullptr, "Data pointer is null.");
    const auto type = array->storage->array_type(array->id);
    const auto size = array->storage->array_size(array->id);
    array->storage->array_read(
        array->id,
        {static_cast<std::byte*>(data), size * type.width()});
    return 0;
  });
}

struct ttdb_array_iter {
  StoragePtr storage;
  InputStreamPtr<ArrayID> ids;
};

void ttdb_array_iter__close(ttdb_array_iter_t* iter) {
  delete iter;
}

ttdb_array_t* ttdb_array_iter__next(ttdb_array_iter_t* iter) {
  return safe_call([iter] -> ttdb_array_t* {
    TIT_ENSURE(iter != nullptr, "Array iterator pointer is null.");
    const auto id = iter->ids->read();
    if (!id.has_value()) return nullptr;
    return new ttdb_array{
        .storage = iter->storage,
        .id = id.value(),
        .name = iter->storage->array_name(id.value()),
    };
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct ttdb_frame {
  StoragePtr storage;
  FrameID id;
};

void ttdb_frame__close(ttdb_frame_t* frame) {
  delete frame;
}

double ttdb_frame__time(ttdb_frame_t* frame) {
  return safe_call([frame] {
    TIT_ENSURE(frame != nullptr, "Frame pointer is null.");
    return frame->storage->frame_time(frame->id);
  });
}

uint64_t ttdb_frame__num_arrays(ttdb_frame_t* frame) {
  return safe_call([frame] {
    TIT_ENSURE(frame != nullptr, "Frame pointer is null.");
    return frame->storage->frame_num_arrays(frame->id);
  });
}

ttdb_array_t* ttdb_frame__find_array(ttdb_frame_t* frame, const char* name) {
  return safe_call([frame, name] -> ttdb_array_t* {
    TIT_ENSURE(frame != nullptr, "Frame pointer is null.");
    TIT_ENSURE(name != nullptr, "Name pointer is null.");
    const auto id = frame->storage->frame_find_array_id(frame->id, name);
    if (!id.has_value()) return nullptr;
    return new ttdb_array{
        .storage = frame->storage,
        .id = id.value(),
        .name = name,
    };
  });
}

ttdb_array_iter_t* ttdb_frame__arrays(ttdb_frame_t* frame) {
  return safe_call([frame] {
    TIT_ENSURE(frame != nullptr, "Frame pointer is null.");
    return new ttdb_array_iter{
        .storage = frame->storage,
        .ids = make_range_input_stream( //
            frame->storage->frame_array_ids(frame->id)),
    };
  });
}

struct ttdb_frame_iter {
  std::shared_ptr<Storage> storage;
  InputStreamPtr<FrameID> ids;
};

void ttdb_frame_iter__close(ttdb_frame_iter_t* iter) {
  delete iter;
}

ttdb_frame_t* ttdb_frame_iter__next(ttdb_frame_iter_t* iter) {
  return safe_call([iter] -> ttdb_frame_t* {
    TIT_ENSURE(iter != nullptr, "Frame iterator pointer is null.");
    const auto id = iter->ids->read();
    if (!id.has_value()) return nullptr;
    return new ttdb_frame{.storage = iter->storage, .id = id.value()};
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct ttdb_series {
  std::shared_ptr<Storage> storage;
  SeriesID id;
};

void ttdb_series__close(ttdb_series_t* series) {
  delete series;
}

uint64_t ttdb_series__num_frames(ttdb_series_t* series) {
  return safe_call([series] {
    TIT_ENSURE(series != nullptr, "Series pointer is null.");
    return series->storage->series_num_frames(series->id);
  });
}

ttdb_frame_t* ttdb_series__last_frame(ttdb_series_t* series) {
  return safe_call([series] {
    TIT_ENSURE(series != nullptr, "Series pointer is null.");
    return new ttdb_frame{
        .storage = series->storage,
        .id = series->storage->series_last_frame_id(series->id),
    };
  });
}

ttdb_frame_iter_t* ttdb_series__frames(ttdb_series_t* series) {
  return safe_call([series] {
    TIT_ENSURE(series != nullptr, "Series pointer is null.");
    return new ttdb_frame_iter{
        .storage = series->storage,
        .ids = make_range_input_stream(
            series->storage->series_frame_ids(series->id)),
    };
  });
}

struct ttdb_series_iter {
  StoragePtr storage;
  InputStreamPtr<SeriesID> ids;
};

void ttdb_series_iter__close(ttdb_series_iter_t* iter) {
  delete iter;
}

ttdb_series_t* ttdb_series_iter__next(ttdb_series_iter_t* iter) {
  return safe_call([iter] -> ttdb_series_t* {
    TIT_ENSURE(iter != nullptr, "Series iterator pointer is null.");
    const auto id = iter->ids->read();
    if (!id.has_value()) return nullptr;
    return new ttdb_series{.storage = iter->storage, .id = id.value()};
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct ttdb {
  StoragePtr storage;
};

void ttdb__close(ttdb_t* db) {
  delete db;
}

ttdb_t* ttdb__open(const char* path) {
  return safe_call([path] {
    TIT_ENSURE(path != nullptr, "Path pointer is null.");
    return new ttdb{
        .storage = std::make_shared<Storage>(path, /*read_only=*/true),
    };
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
        .id = db->storage->last_series_id(),
    };
  });
}

ttdb_series_iter_t* ttdb__series(ttdb_t* db) {
  return safe_call([db] {
    TIT_ENSURE(db != nullptr, "Database pointer is null.");
    return new ttdb_series_iter{
        .storage = db->storage,
        .ids = make_range_input_stream(db->storage->series_ids()),
    };
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
