/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <napi.h>

#include "tit/core/exception.hpp"
#include "tit/io/run.hpp"

namespace tit::gui::run {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Path-backed, serialized access to a live run directory.
class RunHolder final {
public:

  explicit RunHolder(std::filesystem::path path) : path_{std::move(path)} {}

  /// Return zero until the solver has published the run metadata and index.
  auto frame_count() const -> std::size_t {
    const std::scoped_lock lock{mutex_};
    if (!std::filesystem::is_regular_file(path_ / "manifest.json") ||
        !std::filesystem::is_regular_file(path_ / "index.json")) {
      return 0;
    }
    return io::RunReader{path_}.num_frames();
  }

  /// Open a fresh logical reader while holding the per-run native lock.
  auto access(const auto& callback) const {
    const std::scoped_lock lock{mutex_};
    TIT_ENSURE(std::filesystem::is_regular_file(path_ / "manifest.json") &&
                   std::filesystem::is_regular_file(path_ / "index.json"),
               "Run '{}' is not available yet.",
               path_.string());
    const io::RunReader reader{path_};
    return std::invoke(callback, reader);
  }

private:

  std::filesystem::path path_;
  mutable std::mutex mutex_;

}; // class RunHolder

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Open a path-backed run handle without requiring the solver to have started.
auto openRun(const Napi::CallbackInfo& info) -> Napi::Value;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// JavaScript wrapper around a logical `tit::io::RunReader`.
class RunWrap final : public Napi::ObjectWrap<RunWrap> {
public:

  static void init(Napi::Env env);
  static auto New(Napi::Env env, std::shared_ptr<RunHolder> holder)
      -> Napi::Object;

  auto metadata(const Napi::CallbackInfo& info) -> Napi::Value;
  auto frameCount(const Napi::CallbackInfo& info) -> Napi::Value;
  auto frame(const Napi::CallbackInfo& info) -> Napi::Value;
  auto exportTo(const Napi::CallbackInfo& info) -> Napi::Value;

private:

  friend class Napi::ObjectWrap<RunWrap>;
  using Napi::ObjectWrap<RunWrap>::ObjectWrap;

  static auto constructor() -> Napi::FunctionReference&;

  std::shared_ptr<RunHolder> holder_;

}; // class RunWrap

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// JavaScript wrapper around an immutable committed frame.
class FrameWrap final : public Napi::ObjectWrap<FrameWrap> {
public:

  static void init(Napi::Env env);
  static auto New(Napi::Env env,
                  std::shared_ptr<RunHolder> holder,
                  std::size_t frame_index,
                  io::FrameDescriptor descriptor) -> Napi::Object;

  auto descriptor(const Napi::CallbackInfo& info) -> Napi::Value;
  auto fields(const Napi::CallbackInfo& info) -> Napi::Value;
  auto field(const Napi::CallbackInfo& info) -> Napi::Value;

private:

  friend class Napi::ObjectWrap<FrameWrap>;
  using Napi::ObjectWrap<FrameWrap>::ObjectWrap;

  static auto constructor() -> Napi::FunctionReference&;

  std::shared_ptr<RunHolder> holder_;
  std::size_t frame_index_ = 0;
  std::uint64_t step_ = 0;
  double time_ = 0.0;

}; // class FrameWrap

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// JavaScript wrapper around a logical frame field.
class FieldWrap final : public Napi::ObjectWrap<FieldWrap> {
public:

  static void init(Napi::Env env);
  static auto New(Napi::Env env,
                  std::shared_ptr<RunHolder> holder,
                  std::size_t frame_index,
                  const io::FieldDescriptor& descriptor) -> Napi::Object;

  auto name(const Napi::CallbackInfo& info) -> Napi::Value;
  auto type(const Napi::CallbackInfo& info) -> Napi::Value;
  auto data(const Napi::CallbackInfo& info) -> Napi::Value;

private:

  friend class Napi::ObjectWrap<FieldWrap>;
  using Napi::ObjectWrap<FieldWrap>::ObjectWrap;

  static auto constructor() -> Napi::FunctionReference&;

  std::shared_ptr<RunHolder> holder_;
  std::size_t frame_index_ = 0;
  std::string name_;
  std::uint32_t type_id_ = 0;

}; // class FieldWrap

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Register the run-reader submodule on the addon exports.
void init_submodule(Napi::Env env, Napi::Object exports);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::gui::run
