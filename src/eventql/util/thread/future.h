/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#ifndef _STX_THREAD_FUTURE_H
#define _STX_THREAD_FUTURE_H
#include <functional>
#include <memory>
#include <mutex>
#include <stdlib.h>
#include "eventql/util/autoref.h"
#include "eventql/util/duration.h"
#include "eventql/util/exception.h"
#include "eventql/util/inspect.h"
#include "eventql/util/status.h"
#include "eventql/util/thread/wakeup.h"

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

#include "future_impl.h"
#endif
