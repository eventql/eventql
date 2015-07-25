// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/executor/Scheduler.h>
#include <cortex-base/thread/Wakeup.h>

namespace cortex {

Scheduler::Handle::Handle(Task onFire, std::function<void(Handle*)> onCancel)
    : mutex_(),
      onFire_(onFire),
      onCancel_(onCancel),
      isCancelled_(false) {
}

void Scheduler::Handle::cancel() {
  std::lock_guard<std::mutex> lk(mutex_);

  isCancelled_.store(true);

  if (onCancel_) {
    auto cancelThat = std::move(onCancel_);
    cancelThat(this);
  }
}

void Scheduler::Handle::fire() {
  std::lock_guard<std::mutex> lk(mutex_);

  if (!isCancelled_.load()) {
    onFire_();
  }
}

/**
 * Run the provided task when the wakeup handle is woken up
 */
void Scheduler::executeOnNextWakeup(Task  task, Wakeup* wakeup) {
  executeOnWakeup(task, wakeup, wakeup->generation());
}

/**
 * Run the provided task when the wakeup handle is woken up
 */
void Scheduler::executeOnFirstWakeup(Task task, Wakeup* wakeup) {
  executeOnWakeup(task, wakeup, 0);
}

}  // namespace cortex
