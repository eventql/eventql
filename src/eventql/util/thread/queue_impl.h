/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
