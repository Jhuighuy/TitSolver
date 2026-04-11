/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <napi.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/type.hpp"
#include "tit/data/hdf5.hpp"
#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"
#include "titgui/main/bindings/storage.hpp"
#include "titgui/main/bindings/utils.hpp"

namespace tit::titgui::bindings::storage {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto openStorage(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() >= 1, "Missing argument.");
  TIT_ENSURE(info[0].IsString(), "Argument must be a string.");
  const std::filesystem::path path{info[0].As<Napi::String>().Utf8Value()};

  return enqueue(
      info.Env(),
      [path] {
        return std::make_shared<StorageHolder>(data::Storage{path, true});
      },
      [](Napi::Env env, std::shared_ptr<StorageHolder> state) {
        return StorageWrap::New(env, std::move(state));
      });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto StorageWrap::constructor() -> Napi::FunctionReference& {
  static Napi::FunctionReference constructor_;
  return constructor_;
}

void StorageWrap::init(Napi::Env env) {
  auto ctor = DefineClass( //
      env,
      "Storage",
      {
          InstanceMethod("seriesCount", &StorageWrap::seriesCount),
          InstanceMethod("series", &StorageWrap::series),
          InstanceMethod("lastSeries", &StorageWrap::lastSeries),
      });
  constructor() = Persistent(ctor);
  constructor().SuppressDestruct();
}

auto StorageWrap::New(Napi::Env /*env*/, std::shared_ptr<StorageHolder> holder)
    -> Napi::Object {
  auto object = constructor().New({});
  auto* const self = Napi::ObjectWrap<StorageWrap>::Unwrap(object);
  self->holder_ = std::move(holder);
  return object;
}

auto StorageWrap::seriesCount(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");

  return enqueue(
      info.Env(),
      [holder = holder_] {
        return holder->access(
            [](const data::Storage& storage) { return storage.num_series(); });
      },
      [](Napi::Env env, size_t series_count) {
        return Napi::Number::New(env, static_cast<double>(series_count));
      });
}

auto StorageWrap::series(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() >= 1, "Missing argument.");
  TIT_ENSURE(info[0].IsNumber(), "Argument must be a number.");
  const auto index =
      static_cast<size_t>(info[0].As<Napi::Number>().Int64Value());

  return enqueue(
      info.Env(),
      [holder = holder_, index] {
        return holder->access([index](const data::Storage& storage) {
          return storage.series_id(index);
        });
      },
      [holder = holder_](Napi::Env env, data::SeriesID series_id) {
        return SeriesWrap::New(env, holder, series_id);
      });
}

auto StorageWrap::lastSeries(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");

  return enqueue(
      info.Env(),
      [holder = holder_] {
        return holder->access([](const data::Storage& storage) {
          return storage.last_series_id();
        });
      },
      [holder = holder_](Napi::Env env, data::SeriesID series_id) {
        return SeriesWrap::New(env, holder, series_id);
      });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto SeriesWrap::constructor() -> Napi::FunctionReference& {
  static Napi::FunctionReference constructor_;
  return constructor_;
}

void SeriesWrap::init(Napi::Env env) {
  auto ctor = DefineClass( //
      env,
      "Series",
      {
          InstanceMethod("frameCount", &SeriesWrap::frameCount),
          InstanceMethod("frame", &SeriesWrap::frame),
          InstanceMethod("export", &SeriesWrap::exportTo),
      });
  constructor() = Persistent(ctor);
  constructor().SuppressDestruct();
}

auto SeriesWrap::New(Napi::Env /*env*/,
                     std::shared_ptr<StorageHolder> holder,
                     data::SeriesID series_id) -> Napi::Object {
  auto object = constructor().New({});
  auto* const self = Napi::ObjectWrap<SeriesWrap>::Unwrap(object);
  self->holder_ = std::move(holder);
  self->series_id_ = series_id;
  return object;
}

auto SeriesWrap::frameCount(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");

  return enqueue(
      info.Env(),
      [holder = holder_, series_id = series_id_] {
        return holder->access([series_id](const data::Storage& storage) {
          return storage.series_num_frames(series_id);
        });
      },
      [](Napi::Env env, size_t frame_count) {
        return Napi::Number::New(env, static_cast<double>(frame_count));
      });
}

auto SeriesWrap::frame(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() >= 1, "Missing argument.");
  TIT_ENSURE(info[0].IsNumber(), "Argument must be a number.");
  const auto index =
      static_cast<size_t>(info[0].As<Napi::Number>().Int64Value());

  return enqueue(
      info.Env(),
      [state = holder_, series_id = series_id_, index] {
        return state->access([series_id, index](const data::Storage& storage) {
          return storage.series_frame_id(series_id, index);
        });
      },
      [state = holder_](Napi::Env env, data::FrameID frame_id) {
        return FrameWrap::New(env, state, frame_id);
      });
}

auto SeriesWrap::exportTo(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() >= 1, "Missing argument.");
  TIT_ENSURE(info[0].IsString(), "Argument must be a string.");
  const std::filesystem::path dir_path{info[0].As<Napi::String>().Utf8Value()};

  return enqueue(
      info.Env(),
      [state = holder_, series_id = series_id_, dir_path] {
        return state->access(
            [series_id, &dir_path](const data::Storage& storage) {
              std::filesystem::create_directories(dir_path);
              data::export_hdf5(
                  dir_path,
                  data::SeriesView<const data::Storage>{storage, series_id});
              return std::monostate{};
            });
      },
      [](Napi::Env env, std::monostate) { return env.Undefined(); });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto FrameWrap::constructor() -> Napi::FunctionReference& {
  static Napi::FunctionReference constructor_;
  return constructor_;
}

void FrameWrap::init(Napi::Env env) {
  auto ctor = DefineClass( //
      env,
      "Frame",
      {
          InstanceMethod("fields", &FrameWrap::fields),
          InstanceMethod("field", &FrameWrap::field),
      });
  constructor() = Persistent(ctor);
  constructor().SuppressDestruct();
}

auto FrameWrap::New(Napi::Env /*env*/,
                    std::shared_ptr<StorageHolder> holder,
                    data::FrameID frame_id) -> Napi::Object {
  auto object = constructor().New({});
  auto* const self = Napi::ObjectWrap<FrameWrap>::Unwrap(object);
  self->holder_ = std::move(holder);
  self->frame_id_ = frame_id;
  return object;
}

auto FrameWrap::fields(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");

  return enqueue(
      info.Env(),
      [holder = holder_, frame_id = frame_id_] {
        return holder->access([frame_id](const data::Storage& storage) {
          return storage.frame_arrays(frame_id) |
                 std::ranges::views::transform(
                     &data::ArrayView<const data::Storage>::name) |
                 std::ranges::to<std::vector>();
        });
      },
      [](Napi::Env env, std::vector<std::string> names) {
        auto result = Napi::Array::New(env, names.size());
        for (uint32_t index = 0; index < names.size(); index++) {
          result.Set(index, names[index]);
        }
        return result;
      });
}

auto FrameWrap::field(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 1, "Unexpected arguments.");
  TIT_ENSURE(info[0].IsString(), "Argument must be a string.");
  const auto name = info[0].As<Napi::String>().Utf8Value();

  return enqueue(
      info.Env(),
      [holder = holder_, frame_id = frame_id_, name] {
        return holder->access([frame_id, &name](const data::Storage& storage) {
          const auto array_id = storage.frame_find_array_id(frame_id, name);
          TIT_ENSURE(array_id.has_value(), "Unknown frame field: {}.", name);
          return *array_id;
        });
      },
      [holder = holder_](Napi::Env env, data::ArrayID array_id) {
        return FieldWrap::New(env, holder, array_id);
      });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto FieldWrap::constructor() -> Napi::FunctionReference& {
  static Napi::FunctionReference constructor_;
  return constructor_;
}

void FieldWrap::init(Napi::Env env) {
  auto ctor = DefineClass( //
      env,
      "Field",
      {
          InstanceMethod("name", &FieldWrap::name),
          InstanceMethod("type", &FieldWrap::type),
          InstanceMethod("data", &FieldWrap::data),
      });
  constructor() = Persistent(ctor);
  constructor().SuppressDestruct();
}

auto FieldWrap::New(Napi::Env /*env*/,
                    std::shared_ptr<StorageHolder> holder,
                    data::ArrayID array_id) -> Napi::Object {
  auto object = constructor().New({});
  auto* const self = Napi::ObjectWrap<FieldWrap>::Unwrap(object);
  self->holder_ = std::move(holder);
  self->array_id_ = array_id;
  return object;
}

auto FieldWrap::name(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");

  return enqueue(
      info.Env(),
      [holder = holder_, array_id = array_id_] {
        return holder->access([array_id](const data::Storage& storage) {
          return storage.array_name(array_id);
        });
      },
      [](Napi::Env env, const std::string& name) {
        return Napi::String::New(env, name);
      });
}

auto FieldWrap::type(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");

  return enqueue(
      info.Env(),
      [holder = holder_, array_id = array_id_] {
        return holder->access([array_id](const data::Storage& storage) {
          return storage.array_type(array_id);
        });
      },
      [](Napi::Env env, const data::Type& type) {
        // Patch the kind to be consistent with the convention below.
        auto kind = type.kind();
        if (kind.id() == data::Kind::ID::int64 ||
            kind.id() == data::Kind::ID::uint64) {
          kind = data::kind_of<float64_t>;
        }

        auto object = Napi::Object::New(env);
        object.Set("kind", kind.name());
        object.Set("rank", std::to_underlying(type.rank()));
        object.Set("dim", static_cast<double>(type.dim()));

        return object;
      });
}

auto FieldWrap::data(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");

  return enqueue(
      info.Env(),
      [holder = holder_, array_id = array_id_] {
        // Read the array data under lock.
        auto [type, data] =
            holder->access([array_id](const data::Storage& storage) {
              return std::make_pair(storage.array_type(array_id),
                                    storage.array_read(array_id));
            });

        // JS doesn't support 64-bit integers, so we have to make a conversion.
        if (type.kind().id() == data::Kind::ID::int64 ||
            type.kind().id() == data::Kind::ID::uint64) {
          // Convert to floats in-place.
          auto* const out = safe_bit_ptr_cast<float64_t*>(data.data());
          if (type.kind().id() == data::Kind::ID::int64) {
            const std::span<const int64_t> in{
                safe_bit_ptr_cast<const int64_t*>(data.data()),
                data.size() / sizeof(int64_t),
            };
            std::ranges::transform(in, out, [](int64_t value) {
              return static_cast<float64_t>(value);
            });
          } else if (type.kind().id() == data::Kind::ID::uint64) {
            const std::span<const uint64_t> in{
                safe_bit_ptr_cast<const uint64_t*>(data.data()),
                data.size() / sizeof(uint64_t),
            };
            std::ranges::transform(in, out, [](uint64_t value) {
              return static_cast<float64_t>(value);
            });
          } else {
            std::unreachable();
          }

          // Patch the type.
          type = data::Type{
              data::kind_of<float64_t>,
              type.rank(),
              static_cast<uint8_t>(type.dim()),
          };
        }

        return std::make_pair(type, std::move(data));
      },
      [](Napi::Env env,
         std::pair<data::Type, std::vector<std::byte>> type_data) {
        auto [type, data] = std::move(type_data);

        // Create a buffer and copy the data into it.
        auto buffer = Napi::ArrayBuffer::New(env, data.size());
        std::memcpy(buffer.Data(), data.data(), data.size());

        // Create the typed array.
        const auto length = data.size() / type.kind().width();
        using enum data::Kind::ID;
        switch (type.kind().id()) {
          case int8:
            return Napi::Int8Array::New(env, length, buffer, 0)
                .As<Napi::Value>();
          case uint8:
            return Napi::Uint8Array::New(env, length, buffer, 0)
                .As<Napi::Value>();
          case int16:
            return Napi::Int16Array::New(env, length, buffer, 0)
                .As<Napi::Value>();
          case uint16:
            return Napi::Uint16Array::New(env, length, buffer, 0)
                .As<Napi::Value>();
          case int32:
            return Napi::Int32Array::New(env, length, buffer, 0)
                .As<Napi::Value>();
          case uint32:
            return Napi::Uint32Array::New(env, length, buffer, 0)
                .As<Napi::Value>();
          case float32:
            return Napi::Float32Array::New(env, length, buffer, 0)
                .As<Napi::Value>();
          case float64:
            return Napi::Float64Array::New(env, length, buffer, 0)
                .As<Napi::Value>();
          default: TIT_THROW("Unsupported field kind.");
        }
      });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void init_submodule(Napi::Env env, Napi::Object exports) {
  StorageWrap::init(env);
  SeriesWrap::init(env);
  FrameWrap::init(env);
  FieldWrap::init(env);
  exports.Set("openStorage", Napi::Function::New(env, openStorage));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::titgui::bindings::storage
