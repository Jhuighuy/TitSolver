/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <memory>
#include <mutex>
#include <utility>

#include <napi.h>

#include "tit/data/storage.hpp"

namespace tit::titgui::bindings::storage {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Storage object with mutex.
class StorageHolder final : public std::enable_shared_from_this<StorageHolder> {
public:

  /// Construct a storage state.
  explicit StorageHolder(data::Storage storage)
      : storage_{std::move(storage)} {}

  /// Access the storage.
  auto access(const auto& callback) const {
    const std::scoped_lock lock{mutex_};
    return std::invoke(callback, storage_);
  }

private:

  data::Storage storage_;
  mutable std::mutex mutex_;

}; // class StorageState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Open a storage and return a promise resolving to a wrapped object.
auto openStorage(const Napi::CallbackInfo& info) -> Napi::Value;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// JavaScript wrapper around `tit::data::Storage`.
class StorageWrap final : public Napi::ObjectWrap<StorageWrap> {
public:

  /// Register the wrapped storage class.
  static void init(Napi::Env env);

  /// Create a wrapped storage object.
  static auto New(Napi::Env env, std::shared_ptr<StorageHolder> holder)
      -> Napi::Object;

  auto seriesCount(const Napi::CallbackInfo& info) -> Napi::Value;
  auto series(const Napi::CallbackInfo& info) -> Napi::Value;
  auto lastSeries(const Napi::CallbackInfo& info) -> Napi::Value;

private:

  friend class Napi::ObjectWrap<StorageWrap>;

  static auto constructor() -> Napi::FunctionReference&;
  using Napi::ObjectWrap<StorageWrap>::ObjectWrap;

  std::shared_ptr<StorageHolder> holder_;

}; // class StorageWrap

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// JavaScript wrapper around `tit::data::SeriesView`.
class SeriesWrap final : public Napi::ObjectWrap<SeriesWrap> {
public:

  /// Register the wrapped series class.
  static void init(Napi::Env env);

  /// Create a wrapped series object.
  static auto New(Napi::Env env,
                  std::shared_ptr<StorageHolder> holder,
                  data::SeriesID series_id) -> Napi::Object;

  auto frameCount(const Napi::CallbackInfo& info) -> Napi::Value;
  auto frame(const Napi::CallbackInfo& info) -> Napi::Value;
  auto exportTo(const Napi::CallbackInfo& info) -> Napi::Value;

private:

  friend class Napi::ObjectWrap<SeriesWrap>;
  using Napi::ObjectWrap<SeriesWrap>::ObjectWrap;

  static auto constructor() -> Napi::FunctionReference&;

  std::shared_ptr<StorageHolder> holder_;
  data::SeriesID series_id_{};

}; // class SeriesWrap

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// JavaScript wrapper around a storage frame.
class FrameWrap final : public Napi::ObjectWrap<FrameWrap> {
public:

  /// Register the wrapped frame class.
  static void init(Napi::Env env);

  /// Create a wrapped frame object.
  static auto New(Napi::Env env,
                  std::shared_ptr<StorageHolder> holder,
                  data::FrameID frame_id) -> Napi::Object;

  auto fields(const Napi::CallbackInfo& info) -> Napi::Value;
  auto field(const Napi::CallbackInfo& info) -> Napi::Value;

private:

  friend class Napi::ObjectWrap<FrameWrap>;
  using Napi::ObjectWrap<FrameWrap>::ObjectWrap;

  static auto constructor() -> Napi::FunctionReference&;

  std::shared_ptr<StorageHolder> holder_;
  data::FrameID frame_id_{};

}; // class FrameWrap

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// JavaScript wrapper around a storage field.
class FieldWrap final : public Napi::ObjectWrap<FieldWrap> {
public:

  /// Register the wrapped field class.
  static void init(Napi::Env env);

  /// Create a wrapped field object.
  static auto New(Napi::Env env,
                  std::shared_ptr<StorageHolder> holder,
                  data::ArrayID array_id) -> Napi::Object;

  auto name(const Napi::CallbackInfo& info) -> Napi::Value;
  auto type(const Napi::CallbackInfo& info) -> Napi::Value;
  auto data(const Napi::CallbackInfo& info) -> Napi::Value;

private:

  friend class Napi::ObjectWrap<FieldWrap>;
  using Napi::ObjectWrap<FieldWrap>::ObjectWrap;

  static auto constructor() -> Napi::FunctionReference&;

  std::shared_ptr<StorageHolder> holder_;
  data::ArrayID array_id_{};

}; // class FieldWrap

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Register the storage submodule on the addon exports.
void init_submodule(Napi::Env env, Napi::Object exports);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::titgui::bindings::storage
