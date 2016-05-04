/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/wallclock.h>

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
