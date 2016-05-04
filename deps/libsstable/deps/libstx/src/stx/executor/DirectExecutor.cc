// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <stx/executor/DirectExecutor.h>
#include <stx/logging.h>
#include <stdio.h>

namespace stx {

#ifndef NDEBUG
#define TRACE(msg...) logTrace("executor.DirectExecutor", msg)
#else
#define TRACE(msg...) do {} while (0)
#endif

DirectExecutor::DirectExecutor(
    bool recursive,
    std::unique_ptr<stx::ExceptionHandler> eh)
    : Executor(std::move(eh)),
      recursive_(recursive),
      running_(0),
      deferred_() {
}

void DirectExecutor::execute(Task task) {
  if (isRunning() && !isRecursive()) {
    deferred_.emplace_back(std::move(task));
    TRACE("%p execute: enqueue task (%zu)", this, deferred_.size());
    return;
  }

  running_++;

  TRACE("%p execute: run top-level task", this);
  safeCall(task);

  while (!deferred_.empty()) {
    TRACE("%p execute: run deferred task (-%zu)", this, deferred_.size());
    safeCall(deferred_.front());
    deferred_.pop_front();
  }

  running_--;
}

std::string DirectExecutor::toString() const {
  char buf[128];
  snprintf(buf, sizeof(buf), "DirectExecutor@%p", this);
  return buf;
}

} // namespace stx
