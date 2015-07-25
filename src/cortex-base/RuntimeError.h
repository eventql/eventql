// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <cortex-base/Status.h>
#include <cortex-base/StackTrace.h>
#include <cortex-base/sysconfig.h>
#include <iosfwd>
#include <vector>
#include <string>
#include <stdexcept>
#include <system_error>
#include <string.h>
#include <errno.h>

namespace cortex {

class CORTEX_API RuntimeError : public std::system_error {
 public:
  RuntimeError(int ev, const std::error_category& ec);
  RuntimeError(int ev, const std::error_category& ec, const std::string& what);

  explicit RuntimeError(Status ev);
  RuntimeError(Status ev, const std::string& what);

  template<typename... Args>
  RuntimeError(Status ev, const char* fmt, Args... args);

  ~RuntimeError();

  template<typename T = RuntimeError>
  T setSource(const char* file, int line, const char* fn);
  const char* sourceFile() const { return sourceFile_; }
  int sourceLine() const CORTEX_NOEXCEPT { return sourceLine_; }
  const char* functionName() const { return functionName_; }

  // XXX for backwards-compatibility only
  CORTEX_DEPRECATED const char* typeName() const;
  CORTEX_DEPRECATED bool ofType(Status ev) const;

  std::vector<std::string> backtrace() const;

  void debugPrint(std::ostream* os = nullptr) const;

 private:
  static std::string cformat(const char* fmt, ...);

 private:
  const char* sourceFile_;
  int sourceLine_;
  const char* functionName_;
  StackTrace stackTrace_;
};

CORTEX_API void logAndPass(const std::exception& e);
CORTEX_API void logAndAbort(const std::exception& e);

// {{{ inlines
template<typename T>
inline T RuntimeError::setSource(const char* file, int line, const char* fn) {
  sourceFile_ = file;
  sourceLine_ = line;
  functionName_ = fn;
  return T(*this);
}


template<typename... Args>
inline RuntimeError::RuntimeError(
    Status ev,
    const char* fmt,
    Args... args)
  : RuntimeError((int) ev,
                 StatusCategory::get(),
                 cformat(fmt, args...)) {
}
// }}}

} // namespace cortex

#define EXCEPTION(E, ...)                                                     \
  (E(__VA_ARGS__).setSource<E>(__FILE__, __LINE__, __PRETTY_FUNCTION__))

/**
 * Raises an exception of given type and arguments being passed to the
 * constructor.
 *
 * The exception must derive from RuntimeError, whose additional
 * source information will be initialized after.
 */
#define RAISE_EXCEPTION(E, ...) {                                             \
  throw E(__VA_ARGS__).setSource<E>(__FILE__, __LINE__, __PRETTY_FUNCTION__); \
}

/**
 * Raises an exception of type RuntimeError for given code and category.
 *
 * Also sets source file, line, and calling function.
 *
 * @param code 1st argument is the error value.
 * @param category 2nd argument is the error category.
 * @param what_arg 3rd (optional) argument is the actual error message.
 */
#define RAISE_CATEGORY(Code, ...) {                                           \
  RAISE_EXCEPTION(RuntimeError, ((int) Code), __VA_ARGS__);                   \
}

/**
 * Raises an exception of given operating system error code.
 *
 * @see RAISE_EXCEPTION(E, ...)
 */
#define RAISE_ERRNO(errno) {                                                  \
  RAISE_CATEGORY(errno, std::system_category());                              \
}

/**
 * Raises a RuntimeError for error codes of type Status.
 *
 * @param StatusCode must be a member field of Status.
 */
#define RAISE_STATUS(StatusCode)                                              \
  RAISE_CATEGORY((Status:: StatusCode), StatusCategory::get())

/**
 * Alias to RAISE_STATUS(StatusCode).
 *
 * @param StatusCode must be a member of the enum class Status.
 */
#define RAISE(...) {                                                          \
  throw (RuntimeError(Status:: __VA_ARGS__).setSource(__FILE__, __LINE__, __PRETTY_FUNCTION__));  \
}
  //RAISE_EXCEPTION(RuntimeError, (int) Status:: __VA_ARGS__);

/**
 * Raises an exception on given evaluated expression conditional.
 */
#define BUG_ON(cond) {                                                        \
  if (unlikely(cond)) {                                                       \
    RAISE_EXCEPTION(RuntimeError,                                             \
        (int) Status::InternalError,                                          \
        StatusCategory::get(),                                                \
        "BUG ON: (" #cond ")");                                               \
  }                                                                           \
}
