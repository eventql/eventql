/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
namespace stx {
namespace thread {

template <typename T>
Queue<T>::Queue(
    size_t max_size /* = -1 */) :
    max_size_(max_size),
    length_(0) {}

template <typename T>
void Queue<T>::insert(const T& job, bool block /* = false */) {
  std::unique_lock<std::mutex> lk(mutex_);

  if (max_size_ != size_t(-1)) {
    while (length_ >= max_size_) {
      if (!block) {
        RAISE(kRuntimeError, "queue is full");
      }

      wakeup_.wait(lk);
    }
  }

  queue_.emplace_back(job);
  ++length_;
  lk.unlock();
  wakeup_.notify_all();
}

template <typename T>
T Queue<T>::pop() {
  std::unique_lock<std::mutex> lk(mutex_);

  while (queue_.size() == 0) {
    wakeup_.wait(lk);
  }

  auto job = queue_.front();
  queue_.pop_front();
  --length_;
  lk.unlock();
  wakeup_.notify_all();
  return job;
}

template <typename T>
Option<T> Queue<T>::interruptiblePop() {
  std::unique_lock<std::mutex> lk(mutex_);

  if (queue_.size() == 0) {
    wakeup_.wait(lk);
  }

  if (queue_.size() == 0) {
    return None<T>();
  } else {
    auto job = Some(queue_.front());
    queue_.pop_front();
    --length_;
    lk.unlock();
    wakeup_.notify_all();
    return job;
  }
}

template <typename T>
Option<T> Queue<T>::poll() {
  std::unique_lock<std::mutex> lk(mutex_);

  if (queue_.size() == 0) {
    return None<T>();
  } else {
    auto job = Some(queue_.front());
    queue_.pop_front();
    --length_;
    lk.unlock();
    wakeup_.notify_all();
    return job;
  }
}

template <typename T>
size_t Queue<T>::length() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return length_;
}

template <typename T>
void Queue<T>::wakeup() {
  wakeup_.notify_all();
}

template <typename T>
void Queue<T>::waitUntilEmpty() const {
  std::unique_lock<std::mutex> lk(mutex_);

  while (queue_.size() > 0) {
    wakeup_.wait(lk);
  }
}

}
}
