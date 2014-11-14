#include <xzero/executor/Executor.h>
#include <xzero/RuntimeError.h>
#include <typeinfo>

namespace xzero {

Executor::Executor()
    : exceptionHandler_(std::bind(&Executor::standardConsoleLogger,
                                  std::placeholders::_1)) {
}

Executor::~Executor() {
}

void Executor::standardConsoleLogger(const std::exception& e) {
  if (auto rt = dynamic_cast<const RuntimeError*>(&e)) {
    fprintf(stderr, "Unhandled exception caught in executor [%s:%d] (%s): %s\n",
            rt->sourceFile(), rt->sourceLine(), typeid(e).name(), rt->what());
    return;
  }

  fprintf(stderr, "Unhandled exception caught in executor (%s): %s\n",
          typeid(e).name(), e.what());
}

void Executor::setExceptionHandler(
    std::function<void(const std::exception&)>&& eh) {

  exceptionHandler_ = std::move(eh);
}

void Executor::safeCall(const Task& task) noexcept {
  try {
    task();
  } catch (const std::exception& e) {
    handleException(e);
  } catch (...) {
  }
}

void Executor::handleException(const std::exception& e) noexcept {
  if (exceptionHandler_) {
    try {
      exceptionHandler_(e);
    } catch (...) {
    }
  }
}

} // namespace xzero
