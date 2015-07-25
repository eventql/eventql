// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
#pragma once

#include <cortex-base/Api.h>
#include <cortex-base/RefCounted.h>
#include <cortex-base/RefPtr.h>
#include <cortex-base/TimeSpan.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/Status.h>
#include <cortex-base/thread/Wakeup.h>
#include <functional>
#include <memory>
#include <mutex>
#include <cstdlib>

namespace cortex {

class Scheduler;

template <typename> class Future;
template <typename> class Promise;

template <typename T>
class CORTEX_API PromiseState : public RefCounted {
 public:
  PromiseState();
  ~PromiseState();

 private:
  Wakeup wakeup;
  Status status;
  std::mutex mutex; // FIXPAUL use spinlock
  char value_data[sizeof(T)];
  T* value;
  bool ready;

  std::function<void (const Status& status)> on_failure;
  std::function<void (const T& value)> on_success;

  friend class Future<T>;
  friend class Promise<T>;
};

template <typename T>
class CORTEX_API Future {
 public:
  Future(RefPtr<PromiseState<T>> promise_state);
  Future(const Future<T>& other);
  Future(Future<T>&& other);
  ~Future();

  Future& operator=(const Future<T>& other);

  bool isReady() const;
  bool isFailure() const;
  bool isSuccess() const;

  void onFailure(std::function<void (const Status& status)> fn);
  void onSuccess(std::function<void (const T& value)> fn);

  void wait() const;
  void wait(const TimeSpan& timeout) const;

  void onReady(std::function<void> fn);
  void onReady(Scheduler* scheduler, std::function<void> fn);

  T& get();
  const T& get() const;
  const T& waitAndGet() const;

  Wakeup* wakeup() const;

 protected:
  RefPtr<PromiseState<T>> state_;
};

template <typename T>
class CORTEX_API Promise {
 public:
  Promise();
  Promise(const Promise<T>& other);
  Promise(Promise<T>&& other);
  ~Promise();

  void success(const T& value);
  void success(T&& value);
  void failure(const std::exception& e);
  void failure(Status e);

  Future<T> future() const;
  bool isFulfilled() const;

 protected:
  RefPtr<PromiseState<T>> state_;
};

} // namespace cortex

#include <cortex-base/thread/Future-impl.h>
