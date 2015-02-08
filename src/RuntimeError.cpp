// This file is part of the "x0" project
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/RuntimeError.h>
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

#if defined(HAVE_EXECINFO_H)
#include <execinfo.h>
#endif

#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

namespace xzero {

#define MAX_FRAMES 64
#define SKIP_FRAMES 2

class Pipe { // {{{
 private:
  Pipe(const Pipe&) = delete;
  Pipe& operator=(Pipe&) = delete;

 public:
  Pipe();
  Pipe(Pipe&& other);
  Pipe& operator=(Pipe&& other);
  ~Pipe();

  int readFd() const noexcept { return pfd_[0]; }
  int writeFd() const noexcept { return pfd_[1]; }

  void close();
  void closeRead();
  void closeWrite();

 private:
  int pfd_[2];
};

Pipe::Pipe() {
#if defined(HAVE_PIPE2)
  int rv = pipe2(pfd_, O_CLOEXEC);
#else
  int rv = pipe(pfd_);
#endif
  if (rv < 0) {
    throw RUNTIME_ERROR(strerror(errno)); // FIXME: not thread safe
  }
}

Pipe::Pipe(Pipe&& other) {
  pfd_[0] = other.pfd_[0];
  pfd_[1] = other.pfd_[1];

  other.pfd_[0] = -1;
  other.pfd_[1] = -1;
}

Pipe& Pipe::operator=(Pipe&& other) {
  pfd_[0] = other.pfd_[0];
  pfd_[1] = other.pfd_[1];

  other.pfd_[0] = -1;
  other.pfd_[1] = -1;

  return *this;
}

Pipe::~Pipe() {
  close();
}

void Pipe::close() {
  closeRead();
  closeWrite();
}

void Pipe::closeRead() {
  if (readFd() >= 0)
    ::close(readFd());
}

void Pipe::closeWrite() {
  if (writeFd() >= 0)
    ::close(writeFd());
}
// }}}

void consoleLogger(const std::exception& e) {
  if (auto rt = dynamic_cast<const RuntimeError*>(&e)) {
    fprintf(stderr, "Unhandled exception caught [%s:%d] (%s). %s\n",
            rt->sourceFile(), rt->sourceLine(),
            StackTrace::demangleSymbol(typeid(e).name()).c_str(),
            rt->what());
    auto bt = rt->backtrace();
    for (size_t i = 0; i < bt.size(); ++i)
      fprintf(stderr, "  [%zu] %s\n", i, bt[i].c_str());
    return;
  }

  fprintf(stderr, "Unhandled exception caught in executor (%s): %s\n",
          StackTrace::demangleSymbol(typeid(e).name()).c_str(), e.what());
}

RuntimeError::RuntimeError(const std::string& what,
                           const char* sourceFile,
                           int sourceLine)
  : std::runtime_error(what),
    sourceFile_(sourceFile),
    sourceLine_(sourceLine),
    stackTrace_() {
}

RuntimeError::~RuntimeError() {
}

std::vector<std::string> RuntimeError::backtrace() const {
  return stackTrace_.symbols();
}

} // namespace xzero
