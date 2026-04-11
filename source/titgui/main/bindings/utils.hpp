/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <exception>
#include <memory>
#include <optional>
#include <type_traits>

#include <napi.h>

#include "tit/core/exception.hpp"

namespace tit::titgui::bindings {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Promise worker resolving a native result back into JavaScript.
template<std::invocable Func,
         std::invocable<Napi::Env, std::invoke_result_t<Func>> Resolve>
class PromiseWorker final : public Napi::AsyncWorker {
public:

  /// Construct a promise worker.
  PromiseWorker(Napi::Env env, Func func, Resolve resolve)
      : AsyncWorker{env}, deferred_{Napi::Promise::Deferred::New(env)},
        func_{std::move(func)}, resolve_{std::move(resolve)} {}

  /// Queue the worker and return the promise.
  auto queue() -> Napi::Promise {
    Queue();
    return deferred_.Promise();
  }

protected:

  void Execute() override {
    try {
      result_.emplace(std::invoke(func_));
    } catch (const std::exception& error) {
      SetError(error.what());
    }
  }

  void OnOK() override {
    TIT_ENSURE(result_.has_value(), "Promise worker has no result.");
    deferred_.Resolve(std::invoke(resolve_, Env(), std::move(result_).value()));
  }

  void OnError(const Napi::Error& error) override {
    deferred_.Reject(error.Value());
  }

private:

  Napi::Promise::Deferred deferred_;
  [[no_unique_address]] Func func_;
  [[no_unique_address]] Resolve resolve_;
  std::optional<std::invoke_result_t<Func>> result_;

}; // class PromiseWorker

/// Queue a native worker returning a JavaScript promise.
template<std::invocable Func,
         std::invocable<Napi::Env, std::invoke_result_t<Func>> Resolve>
auto enqueue(Napi::Env env, Func func, Resolve resolve) -> Napi::Promise {
  auto worker =
      std::make_unique<PromiseWorker<Func, Resolve>>(env,
                                                     std::move(func),
                                                     std::move(resolve));
  auto promise = worker->queue();
  worker.release();
  return promise;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::titgui::bindings
