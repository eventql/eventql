// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/Option.h>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <deque>

namespace cortex {
namespace thread {

/**
 * A queue is threadsafe
 */
template <typename T>
class CORTEX_API Queue {
public:
  void insert(const T& job);
  T pop();
  Option<T> poll();

protected:
  std::deque<T> queue_;
  std::mutex mutex_;
  std::condition_variable wakeup_;
};

// {{{ inlines
template <typename T>
void Queue<T>::insert(const T& job) {
  std::unique_lock<std::mutex> lk(mutex_);
  queue_.emplace_back(job);
  lk.unlock();
  wakeup_.notify_one();
}

template <typename T>
T Queue<T>::pop() {
  std::unique_lock<std::mutex> lk(mutex_);

  while (queue_.size() == 0) {
    wakeup_.wait(lk);
  }

  auto job = queue_.front();
  queue_.pop_front();
  return job;
}

template <typename T>
Option<T> Queue<T>::poll() {
  std::unique_lock<std::mutex> lk(mutex_);

  if (queue_.size() == 0) {
    return None();
  } else {
    auto job = Some(queue_.front());
    queue_.pop_front();
    return job;
  }
}
// }}}

} // namespace thread
} // namespace cortex
