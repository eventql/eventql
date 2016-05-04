// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <functional>
#include <mutex>

namespace stx {

inline void Scheduler::Handle::reset(Task onCancel) {
  std::lock_guard<std::mutex> lk(mutex_);

  isCancelled_.store(false);
  onCancel_ = onCancel;
}

inline bool Scheduler::Handle::isCancelled() const {
  return isCancelled_.load();
}

inline void Scheduler::Handle::setCancelHandler(Task task) {
  std::lock_guard<std::mutex> lk(mutex_);
  onCancel_ = task;
}

inline void Scheduler::Handle::cancel() {
  std::lock_guard<std::mutex> lk(mutex_);

  for (;;) {
    auto isCancelled = isCancelled_.load();
    if (isCancelled)
      // already cancelled
      break;

    if (isCancelled_.compare_exchange_strong(isCancelled, true)) {
      if (onCancel_) {
        auto cancelThat = std::move(onCancel_);
        cancelThat();
      }
      break;
    }
  }
}

inline void Scheduler::Handle::fire(Task task) {
  std::lock_guard<std::mutex> lk(mutex_);

  if (!isCancelled_.load()) {
    task();
  }
}

inline Scheduler::HandleRef Scheduler::executeOnReadable(int fd, Task task) {
  return executeOnReadable(fd, task, Duration::fromDays(5 * 365), nullptr);
}

inline Scheduler::HandleRef Scheduler::executeOnWritable(int fd, Task task) {
  return executeOnWritable(fd, task, Duration::fromDays(5 * 365), nullptr);
}

}  // namespace stx
