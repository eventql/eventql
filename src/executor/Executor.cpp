#include <xzero/executor/Executor.h>

namespace xzero {

Executor::Executor(std::function<void(const std::exception&)> eh)
    : SafeCall(eh) {
}

Executor::~Executor() {
}

} // namespace xzero
