#include <xzero/executor/DirectExecutor.h>
#include <stdio.h>

namespace xzero {

DirectExecutor::DirectExecutor(bool recursive)
    : recursive_(recursive),
      running_(0),
      deferred_() {
}

void DirectExecutor::execute(Task&& task) {
  if (isRunning() && !isRecursive()) {
    deferred_.push_back(std::move(task));
    return;
  }

  running_++;

  task();

  while (!deferred_.empty()) {
    deferred_.front()();
    deferred_.pop_front();
  }

  running_--;
}

size_t DirectExecutor::maxConcurrency() const noexcept {
  return 1;
}

std::string DirectExecutor::toString() const {
  char buf[128];
  snprintf(buf, sizeof(buf), "DirectExecutor@%p", this);
  return buf;
}

} // namespace xzero
