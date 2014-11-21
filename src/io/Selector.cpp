#include <xzero/io/Selector.h>
#include <xzero/RuntimeError.h>

namespace xzero {

Selector::Selector(std::function<void(const std::exception&)>&& errorLogger)
    : errorLogger_(errorLogger ? errorLogger : &consoleLogger) {
}


void Selector::logError(const std::exception& e) const {
  errorLogger_(e);
}

} // namespace xzero
