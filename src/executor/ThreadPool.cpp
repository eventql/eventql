#include <xzero/executor/ThreadPool.h>

namespace xzero {

// TODO

size_t ThreadPool::maxConcurrency() const noexcept {
  return threads_.size();
}

} // namespace xzero
