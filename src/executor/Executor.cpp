#include <xzero/executor/Executor.h>
#include <xzero/RuntimeError.h>

namespace xzero {

Executor::Executor(
    std::function<void(const std::exception&)>&& eh)
    : exceptionHandler_(std::bind(&consoleLogger, std::placeholders::_1)) {
}

Executor::~Executor() {
}

void Executor::setExceptionHandler(
    std::function<void(const std::exception&)>&& eh) {

  exceptionHandler_ = std::move(eh);
}

void Executor::safeCall(const Task& task) XZERO_NOEXCEPT {
  try {
    if (task) {
      task();
    }
  } catch (const std::exception& e) {
    handleException(e);
  } catch (...) {
  }
}

void Executor::handleException(const std::exception& e) XZERO_NOEXCEPT {
  if (exceptionHandler_) {
    try {
      exceptionHandler_(e);
    } catch (...) {
    }
  }
}

} // namespace xzero
