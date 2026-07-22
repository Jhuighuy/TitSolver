/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <napi.h>

#include "tit/core/exception.hpp"
#include "tit/io/run.hpp"
#include "tit/io/type.hpp"
#include "titgui/bindings/run.hpp"
#include "titgui/bindings/utils.hpp"

namespace tit::gui::run {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto find_field(const io::FrameReader& frame, std::string_view name)
    -> io::FieldDescriptor {
  const auto fields = frame.fields();
  const auto iter = std::ranges::find(fields, name, &io::FieldDescriptor::name);
  TIT_ENSURE(iter != fields.end(), "Unknown frame field '{}'.", name);
  return *iter;
}

auto type_object(Napi::Env env, io::Type type) -> Napi::Object {
  auto object = Napi::Object::New(env);
  object.Set("kind", type.kind().name());
  object.Set("rank", std::to_underlying(type.rank()));
  object.Set("dim", static_cast<double>(type.dim()));
  return object;
}

auto field_data_value(Napi::Env env, io::FieldData field) -> Napi::Value {
  const auto type = field.descriptor().type();
  auto data = std::move(field).data();
  auto buffer = Napi::ArrayBuffer::New(env, data.size());
  if (!data.empty()) std::memcpy(buffer.Data(), data.data(), data.size());

  const auto length = data.size() / type.kind().width();
  using enum io::Kind::ID;
  switch (type.kind().id()) {
    case int8:
      return Napi::Int8Array::New(env, length, buffer, 0).As<Napi::Value>();
    case uint8:
      return Napi::Uint8Array::New(env, length, buffer, 0).As<Napi::Value>();
    case int16:
      return Napi::Int16Array::New(env, length, buffer, 0).As<Napi::Value>();
    case uint16:
      return Napi::Uint16Array::New(env, length, buffer, 0).As<Napi::Value>();
    case int32:
      return Napi::Int32Array::New(env, length, buffer, 0).As<Napi::Value>();
    case uint32:
      return Napi::Uint32Array::New(env, length, buffer, 0).As<Napi::Value>();
    case int64:
      return Napi::BigInt64Array::New(env, length, buffer, 0).As<Napi::Value>();
    case uint64:
      return Napi::BigUint64Array::New(env, length, buffer, 0)
          .As<Napi::Value>();
    case float32:
      return Napi::Float32Array::New(env, length, buffer, 0).As<Napi::Value>();
    case float64:
      return Napi::Float64Array::New(env, length, buffer, 0).As<Napi::Value>();
    default: TIT_THROW("Unsupported field kind.");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

auto openRun(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() >= 1, "Missing argument.");
  TIT_ENSURE(info[0].IsString(), "Argument must be a string.");
  const std::filesystem::path path{info[0].As<Napi::String>().Utf8Value()};

  return enqueue(
      info.Env(),
      [path] { return std::make_shared<RunHolder>(path); },
      [](Napi::Env env, std::shared_ptr<RunHolder> holder) {
        return RunWrap::New(env, std::move(holder));
      });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto RunWrap::constructor() -> Napi::FunctionReference& {
  static Napi::FunctionReference constructor_;
  return constructor_;
}

void RunWrap::init(Napi::Env env) {
  auto ctor = DefineClass( //
      env,
      "Run",
      {
          InstanceMethod("metadata", &RunWrap::metadata),
          InstanceMethod("frameCount", &RunWrap::frameCount),
          InstanceMethod("frame", &RunWrap::frame),
          InstanceMethod("export", &RunWrap::exportTo),
      });
  constructor() = Persistent(ctor);
  constructor().SuppressDestruct();
}

auto RunWrap::New(Napi::Env /*env*/, std::shared_ptr<RunHolder> holder)
    -> Napi::Object {
  auto object = constructor().New({});
  auto* const self = Napi::ObjectWrap<RunWrap>::Unwrap(object);
  self->holder_ = std::move(holder);
  return object;
}

auto RunWrap::metadata(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");
  return enqueue(
      info.Env(),
      [holder = holder_] {
        return holder->access(
            [](const io::RunReader& reader) { return reader.metadata(); });
      },
      [](Napi::Env env, const io::RunMetadata& metadata) {
        auto object = Napi::Object::New(env);
        object.Set("name", metadata.name());
        object.Set("dimension", static_cast<double>(metadata.dimension()));
        return object;
      });
}

auto RunWrap::frameCount(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");
  return enqueue(
      info.Env(),
      [holder = holder_] { return holder->frame_count(); },
      [](Napi::Env env, std::size_t count) {
        return Napi::Number::New(env, static_cast<double>(count));
      });
}

auto RunWrap::frame(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() >= 1, "Missing argument.");
  TIT_ENSURE(info[0].IsNumber(), "Argument must be a number.");
  const auto signed_index = info[0].As<Napi::Number>().Int64Value();
  TIT_ENSURE(signed_index >= 0, "Frame index must not be negative.");
  const auto index = static_cast<std::size_t>(signed_index);

  return enqueue(
      info.Env(),
      [holder = holder_, index] {
        return holder->access([index](const io::RunReader& reader) {
          return reader.frame(index).descriptor();
        });
      },
      [holder = holder_, index](Napi::Env env, io::FrameDescriptor descriptor) {
        return FrameWrap::New(env, holder, index, descriptor);
      });
}

auto RunWrap::exportTo(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() >= 1, "Missing argument.");
  TIT_ENSURE(info[0].IsString(), "Argument must be a string.");
  const std::filesystem::path destination{
      info[0].As<Napi::String>().Utf8Value()};

  return enqueue(
      info.Env(),
      [holder = holder_, destination] {
        return holder->access([&destination](const io::RunReader& reader) {
          reader.copy_to(destination);
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
          InstanceMethod("descriptor", &FrameWrap::descriptor),
          InstanceMethod("fields", &FrameWrap::fields),
          InstanceMethod("field", &FrameWrap::field),
      });
  constructor() = Persistent(ctor);
  constructor().SuppressDestruct();
}

auto FrameWrap::New(Napi::Env /*env*/,
                    std::shared_ptr<RunHolder> holder,
                    std::size_t frame_index,
                    io::FrameDescriptor descriptor) -> Napi::Object {
  auto object = constructor().New({});
  auto* const self = Napi::ObjectWrap<FrameWrap>::Unwrap(object);
  self->holder_ = std::move(holder);
  self->frame_index_ = frame_index;
  self->step_ = descriptor.step();
  self->time_ = descriptor.time();
  return object;
}

// Node-API requires a non-const member-function callback.
// NOLINTNEXTLINE(readability-make-member-function-const)
auto FrameWrap::descriptor(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");
  return enqueue(
      info.Env(),
      [step = step_, time = time_] { return io::FrameDescriptor{step, time}; },
      [](Napi::Env env, io::FrameDescriptor descriptor) {
        auto object = Napi::Object::New(env);
        object.Set("step", Napi::BigInt::New(env, descriptor.step()));
        object.Set("time", descriptor.time());
        return object;
      });
}

auto FrameWrap::fields(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");
  return enqueue(
      info.Env(),
      [holder = holder_, index = frame_index_] {
        return holder->access([index](const io::RunReader& reader) {
          return reader.frame(index).fields();
        });
      },
      [](Napi::Env env, const std::vector<io::FieldDescriptor>& fields) {
        auto result = Napi::Array::New(env, fields.size());
        for (std::uint32_t index = 0; index < fields.size(); ++index) {
          result.Set(index, fields[index].name());
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
      [holder = holder_, index = frame_index_, name] {
        return holder->access([index, &name](const io::RunReader& reader) {
          return find_field(reader.frame(index), name);
        });
      },
      [holder = holder_,
       index = frame_index_](Napi::Env env,
                             const io::FieldDescriptor& descriptor) {
        return FieldWrap::New(env, holder, index, descriptor);
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
                    std::shared_ptr<RunHolder> holder,
                    std::size_t frame_index,
                    const io::FieldDescriptor& descriptor) -> Napi::Object {
  auto object = constructor().New({});
  auto* const self = Napi::ObjectWrap<FieldWrap>::Unwrap(object);
  self->holder_ = std::move(holder);
  self->frame_index_ = frame_index;
  self->name_ = descriptor.name();
  self->type_id_ = descriptor.type().id();
  return object;
}

auto FieldWrap::name(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");
  return enqueue(
      info.Env(),
      [name = name_] { return name; },
      [](Napi::Env env, const std::string& name) {
        return Napi::String::New(env, name);
      });
}

// Node-API requires a non-const member-function callback.
// NOLINTNEXTLINE(readability-make-member-function-const)
auto FieldWrap::type(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");
  TIT_ENSURE(type_id_ != 0, "Field wrapper has no type.");
  return enqueue(
      info.Env(),
      [type_id = type_id_] { return io::Type{type_id}; },
      [](Napi::Env env, io::Type type) { return type_object(env, type); });
}

auto FieldWrap::data(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");
  return enqueue(
      info.Env(),
      [holder = holder_, index = frame_index_, name = name_] {
        return holder->access([index, &name](const io::RunReader& reader) {
          return reader.frame(index).read_field(name);
        });
      },
      [](Napi::Env env, io::FieldData field) {
        return field_data_value(env, std::move(field));
      });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void init_submodule(Napi::Env env, Napi::Object exports) {
  RunWrap::init(env);
  FrameWrap::init(env);
  FieldWrap::init(env);
  exports.Set("openRun", Napi::Function::New(env, openRun));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::gui::run
