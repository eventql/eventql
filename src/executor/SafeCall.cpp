#include <xzero/executor/SafeCall.h>
#include <xzero/RuntimeError.h>
#include <exception>

namespace xzero {

SafeCall::SafeCall()
    : SafeCall(std::bind(&consoleLogger, std::placeholders::_1)) {
}

SafeCall::SafeCall(std::function<void(const std::exception&)> eh)
    : exceptionHandler_(std::move(eh)) {
}

void SafeCall::setExceptionHandler(
    std::function<void(const std::exception&)> eh) {

  exceptionHandler_ = std::move(eh);
}

void SafeCall::safeCall(std::function<void()> task) XZERO_NOEXCEPT {
  try {
    if (task) {
      task();
    }
  } catch (const std::exception& e) {
    handleException(e);
  } catch (...) {
  }
}

void SafeCall::handleException(const std::exception& e) XZERO_NOEXCEPT {
  if (exceptionHandler_) {
    try {
      exceptionHandler_(e);
    } catch (...) {
    }
  }
}

} // namespace xzero
