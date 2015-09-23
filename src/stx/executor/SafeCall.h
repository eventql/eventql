// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <stx/sysconfig.h>
#include <stx/exceptionhandler.h>

#include <exception>
#include <deque>
#include <functional>
#include <string>

namespace stx {

/**
 * helper class for safely invoking callbacks with regards to unhandled
 * exceptions.
 *
 * Unhandled exceptions will be caught and passed to the exception handler,
 * i.e. to log them.
 */
class SafeCall {
 public:
  SafeCall();

  explicit SafeCall(std::unique_ptr<ExceptionHandler> eh);

  /**
   * Configures exception handler.
   */
  void setExceptionHandler(std::unique_ptr<ExceptionHandler> eh);

  /**
   * Savely invokes given task within the callers context.
   *
   * A call to this function will never leak an unhandled exception.
   *
   * @see setExceptionHandler(std::function<void(const std::exception&)>)
   */
  void safeCall(std::function<void()> callee) noexcept;

  /**
   * Convinience call operator.
   *
   * @see void safeCall(std::function<void()> callee)
   */
  void operator()(std::function<void()> callee) noexcept {
    safeCall(callee);
  }

 protected:
  /**
   * Handles uncaught exception.
   */
  void handleException(const std::exception& e) noexcept;

 private:
  std::unique_ptr<ExceptionHandler> exceptionHandler_;
};

} // namespace stx
