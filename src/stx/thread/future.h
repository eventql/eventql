/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_THREAD_FUTURE_H
#define _STX_THREAD_FUTURE_H
#include <functional>
#include <memory>
#include <mutex>
#include <stdlib.h>
#include "stx/autoref.h"
#include "stx/duration.h"
#include "stx/exception.h"
#include "stx/inspect.h"
#include "stx/status.h"
#include "stx/thread/wakeup.h"

namespace stx {
class TaskScheduler;

template <typename T>
class PromiseState : public RefCounted {
public:
  PromiseState();
  ~PromiseState();

  Status status;
  std::mutex mutex;
  std::condition_variable cv;
  bool ready;

  char value_data[sizeof(T)];
  T* value;

  std::function<void (const Status& status)> on_failure;
  std::function<void (const T& value)> on_success;
};

template <typename T>
class Future {
public:
  Future(AutoRef<PromiseState<T>> promise_state);
  Future(const Future<T>& other);
  Future(Future<T>&& other);
  ~Future();

  Future& operator=(const Future<T>& other);

  bool isReady() const;

  void onFailure(std::function<void (const Status& status)> fn);
  void onSuccess(std::function<void (const T& value)> fn);
  void onReady(std::function<void ()> fn);

  void wait() const;
  bool waitFor(const Duration& timeout) const;

  void onReady(TaskScheduler* scheduler, std::function<void()> fn);

  const T& get() const;
  const T& waitAndGet() const;

  Wakeup* wakeup() const;

protected:
  AutoRef<PromiseState<T>> state_;
};

template <typename T>
class Promise {
public:
  Promise();
  Promise(const Promise<T>& other);
  Promise(Promise<T>&& other);
  ~Promise();

  void success(const T& value);
  void success(T&& value);
  void failure(const std::exception& e);
  void failure(const Status& e);

  Future<T> future() const;
  bool isFulfilled() const;

protected:
  AutoRef<PromiseState<T>> state_;
};

} // namespace stx

#include "future_impl.h"
#endif
