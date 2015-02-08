// This file is part of the "x0" project
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/StackTrace.h>
#include <xzero/Tokenizer.h>
#include <xzero/Buffer.h>
#include <xzero/sysconfig.h>
#include <typeinfo>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cxxabi.h>

#if defined(HAVE_EXECINFO_H)
#include <execinfo.h>
#endif

#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

namespace xzero {

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

StackTrace::~StackTrace() {
  delete[] frames_;
}

inline std::string StackTrace::demangleSymbol(const char* symbol) {
  int status = 0;
  size_t len = 256;
  char* buf = (char*) malloc(len);

  try {
    char* demangled = abi::__cxa_demangle(symbol, buf, &len, &status);
    if (demangled) {
      std::string result(demangled, len);
      free(buf);
      return result;
    }
  } catch (...) {
  }
  free(buf);
  return symbol;
}

std::vector<std::string> StackTrace::symbols() const {
#if defined(HAVE_DLFCN_H)
  if (frames_ && frameCount_) {
    std::vector<std::string> output;

    for (int i = SKIP_FRAMES; i <= frameCount_; ++i) {
      Dl_info info;
      if (dladdr(frames_[i], &info)) {

        if (info.dli_sname) {
          output.push_back(demangleSymbol(info.dli_sname));
        } else {
          char buf[512];
          int n = snprintf(
              buf,
              sizeof(buf),
              "%s %p",
              info.dli_fname,
              frames_[i]);
          output.push_back(std::string(buf, n));
        }
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

}  // namespace xzero
