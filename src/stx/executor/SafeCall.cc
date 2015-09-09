// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <stx/executor/SafeCall.h>
#include <stx/logging.h>
#include <stx/exceptionhandler.h>
#include <exception>

namespace stx {

SafeCall::SafeCall()
    : SafeCall(std::bind(CatchAndLogExceptionHandler("SafeCall"),
                         std::placeholders::_1)) {
}

SafeCall::SafeCall(std::function<void(const std::exception&)> eh)
    : exceptionHandler_(eh) {
}

void SafeCall::setExceptionHandler(
    std::function<void(const std::exception&)> eh) {

  exceptionHandler_ = eh;
}

void SafeCall::safeCall(std::function<void()> task) noexcept {
  try {
    if (task) {
      task();
    }
  } catch (const std::exception& e) {
    handleException(e);
  } catch (...) {
  }
}

void SafeCall::handleException(const std::exception& e) noexcept {
  if (exceptionHandler_) {
    try {
      exceptionHandler_(e);
    } catch (...) {
    }
  }
}

} // namespace stx
