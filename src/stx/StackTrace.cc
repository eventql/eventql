/**
 * This file is part of the "libstx" project
 *   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
 *   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License v3.0.
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <string.h>
#include <typeinfo>
#include <memory>
#include <functional>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cxxabi.h>
#include "StackTrace.h"
#include <stx/sysconfig.h>

#if defined(HAVE_EXECINFO_H)
#include <execinfo.h>
#endif

#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

namespace stx {

#define MAX_FRAMES 64
#define SKIP_FRAMES 2

StackTrace::StackTrace()
    :
#if defined(HAVE_BACKTRACE)
      frames_(MAX_FRAMES ? new void* [SKIP_FRAMES + MAX_FRAMES] : nullptr),
      frameCount_(MAX_FRAMES ? ::backtrace(frames_, SKIP_FRAMES + MAX_FRAMES)
                              : 0)
#else
      frames_(nullptr),
      frameCount_(0)
#endif
{
}

StackTrace::StackTrace(StackTrace&& other)
    : frames_(other.frames_),
      frameCount_(other.frameCount_) {
  other.frames_ = nullptr;
  other.frameCount_ = 0;
}

StackTrace& StackTrace::operator=(StackTrace&& other) {
  frames_ = other.frames_;
  frameCount_ = other.frameCount_;

  other.frames_ = nullptr;
  other.frameCount_ = 0;

  return *this;
}

StackTrace::StackTrace(const StackTrace& other)
    :
#if defined(HAVE_BACKTRACE)
      frames_(MAX_FRAMES ? new void* [SKIP_FRAMES + MAX_FRAMES] : nullptr),
      frameCount_(other.frameCount_)
#else
      frames_(nullptr),
      frameCount_(0)
#endif
{
  if (frames_ && frameCount_) {
    memcpy(frames_, other.frames_, sizeof(void*) * frameCount_);
  }
}

StackTrace& StackTrace::operator=(const StackTrace& other) {
  delete[] frames_;

#if defined(HAVE_BACKTRACE)
  frames_ = MAX_FRAMES ? new void* [SKIP_FRAMES + MAX_FRAMES] : nullptr;
  frameCount_ = other.frameCount_;

  if (frames_ && frameCount_) {
    memcpy(frames_, other.frames_, sizeof(void*) * frameCount_);
  }
#else
  frames_ = nullptr;
  frameCount_ = 0;
#endif

  return *this;
}

StackTrace::~StackTrace() {
  delete[] frames_;
}

std::string StackTrace::demangleSymbol(const char* symbol) {
  int status = 0;
  char* demangled = abi::__cxa_demangle(symbol, nullptr, 0, &status);

  if (demangled) {
    std::string result(demangled, strlen(demangled));
    free(demangled);
    return result;
  } else {
    return symbol;
  }
}

std::vector<std::string> StackTrace::symbols() const {
#if defined(HAVE_DLFCN_H)
  if (frames_ && frameCount_) {
    std::vector<std::string> output;

    for (int i = SKIP_FRAMES; i <= frameCount_; ++i) {
      Dl_info info;
      if (frames_[i] == 0) {
        continue;
      }

      if (dladdr(frames_[i], &info)) {
        if (info.dli_sname) {
          output.push_back(demangleSymbol(info.dli_sname));
        } else {
          char buf[512];
          int n = snprintf(
              buf,
              sizeof(buf),
              "<%p> in %s",
              info.dli_fname,
              (char*) frames_[i]);
          output.push_back(std::string(buf, n));
        }
      } else {
        char buf[512];
        int n = snprintf(buf, sizeof(buf), "<%p>", frames_[i]);
        output.push_back(std::string(buf, n));
      }
    }

    return output;
  }
#else
  if (frames_ && frameCount_) {
    char** strings = backtrace_symbols(frames_, frameCount_);

    std::vector<std::string> output;
    output.resize(frameCount_);

    for (int i = 0; i < frameCount_; ++i)
      output[i] = strings[i];

    free(strings);

    return output;
  }
#endif
  return {};
}

void StackTrace::debugPrint(int fd) {
  auto syms = symbols();

  for (int i = 0; i < syms.size(); ++i) {
    dprintf(fd, "    at #%i: %s\n", i, syms[i].c_str());
  }
}

}  // namespace stx
