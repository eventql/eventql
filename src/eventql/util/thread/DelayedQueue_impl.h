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
#include <eventql/util/wallclock.h>

namespace stx {
namespace thread {

template <typename T>
DelayedQueue<T>::DelayedQueue(
    size_t max_size /* = -1 */) :
    max_size_(max_size),
    length_(0),
    queue_([] (const Pair<uint64_t, T>& a, const Pair<uint64_t, T>& b) {
      return a.first < b.first;
    }) {}

template <typename T>
void DelayedQueue<T>::insert(
    const T& job,
    UnixTime when,
    bool block /* = false */) {
  std::unique_lock<std::mutex> lk(mutex_);

  if (max_size_ != size_t(-1)) {
    while (length_ >= max_size_) {
      if (!block) {
        RAISE(kRuntimeError, "queue is full");
      }

      wakeup_.wait(lk);
    }
  }

  queue_.emplace(std::make_pair(when.unixMicros(), job));

  ++length_;
  lk.unlock();
  wakeup_.notify_all();
}

template <typename T>
Option<T> DelayedQueue<T>::interruptiblePop() {
  std::unique_lock<std::mutex> lk(mutex_);

  if (queue_.size() == 0) {
    wakeup_.wait(lk);
  }

  if (queue_.size() == 0) {
    return None<T>();
  } else {
    auto now = WallClock::unixMicros();
    if (now < queue_.begin()->first) {
      wakeup_.wait_for(
          lk,
          std::chrono::microseconds(queue_.begin()->first - now));

      return None<T>();
    }

    auto job = Some(queue_.begin()->second);
    queue_.erase(queue_.begin());
    --length_;
    lk.unlock();
    wakeup_.notify_all();
    return job;
  }
}

template <typename T>
size_t DelayedQueue<T>::length() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return length_;
}

template <typename T>
void DelayedQueue<T>::wakeup() {
  wakeup_.notify_all();
}

}
}
