// This file is part of the "x0" project
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/RuntimeError.h>
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

std::string demangleSymbol(const char* symbol) {
  return symbol; // TODO: __cxa_demangle()
}

void consoleLogger(const std::exception& e) {
  if (auto rt = dynamic_cast<const RuntimeError*>(&e)) {
    fprintf(stderr, "Unhandled exception caught in executor [%s:%d] (%s): %s\n",
            rt->sourceFile(), rt->sourceLine(),
            demangleSymbol(typeid(e).name()).c_str(),
            rt->what());
    auto bt = rt->backtrace();
    for (size_t i = 0; i < bt.size(); ++i)
      fprintf(stderr, "  [%zu] %s\n", i, bt[i].c_str());
    return;
  }

  fprintf(stderr, "Unhandled exception caught in executor (%s): %s\n",
          demangleSymbol(typeid(e).name()).c_str(), e.what());
}

RuntimeError::RuntimeError(const std::string& what,
                           const char* sourceFile,
                           int sourceLine)
  : std::runtime_error(what),
    sourceFile_(sourceFile),
    sourceLine_(sourceLine),
#if defined(HAVE_BACKTRACE)
    frames_(MAX_FRAMES ? new void*[SKIP_FRAMES + MAX_FRAMES] : nullptr),
    frameCount_(MAX_FRAMES ? ::backtrace(frames_, SKIP_FRAMES + MAX_FRAMES) : 0)
#else
    frames_(nullptr),
    frameCount_(0)
#endif
{
}

RuntimeError::~RuntimeError() {
  delete[] frames_;
}

std::vector<std::string> RuntimeError::backtrace() const {
#if defined(HAVE_DLFCN_H) && defined(HAVE_DUP2) && defined(HAVE_FORK)
  if (!frameCount_ || !frames_)
    return {};

  std::vector<std::string> output;

  Buffer rawFramesText;
  for (int i = SKIP_FRAMES; i <= frameCount_; ++i) {
    Dl_info info;
    if (dladdr(frames_[i], &info)) {
      char buf[512];
      int n = snprintf(buf, sizeof(buf), "%s %p", info.dli_fname, frames_[i]);
      rawFramesText.push_back(buf, n);
      rawFramesText.push_back('\n');

      if (info.dli_sname)
        n = snprintf(buf, sizeof(buf), "%s", info.dli_sname);
      else
        n = snprintf(buf, sizeof(buf), "%p %s", frames_[i], info.dli_fname);

      output.push_back(std::string(buf, n));
    }
  }

  Pipe fin;
  Pipe fout;

  std::string symbolizerPath = "/usr/bin/llvm-symbolizer";
  if (const char* p = getenv("SYMBOLIZER"))
    symbolizerPath = p;

  pid_t pid = fork();
  if (pid < 0) {
    // error
    return output;
  } else if (pid == 0) {
    // child
    dup2(fin.readFd(), STDIN_FILENO);
    dup2(fout.writeFd(), STDOUT_FILENO);
    dup2(fout.writeFd(), STDERR_FILENO);
    fin.close();
    fout.close();

    execl(symbolizerPath.c_str(), symbolizerPath.c_str(), nullptr);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    abort();
  } else {
    // parent
    fin.closeRead();
    fout.closeWrite();

    write(fin.writeFd(), rawFramesText.data(), rawFramesText.size());
    fin.closeWrite();

    Buffer symbolizerOutput;
    char chunk[1024];
    for (;;) {
      int n = read(fout.readFd(), chunk, sizeof(chunk));
      if (n <= 0)
        break;
      symbolizerOutput.push_back(chunk, n);
    }

    int status = 0;
    waitpid(pid, &status, 0);

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
      // symbolizer failed. return raw output.
      return output;

    if (symbolizerOutput.empty())
      // symbolizer returned nothing
      return output;

    output.clear();
    auto lines = Tokenizer<BufferRef>::tokenize(symbolizerOutput.ref(), "\n");
    for (size_t i = 0; i + 1 < lines.size(); i += 2) {
      Buffer buf;
      buf.push_back(lines[i]);
      buf.push_back(" in ");
      buf.push_back(lines[i + 1]);
      output.push_back(buf.str());
    }
    return output;
  }
#else
  return {};
#endif
}

} // namespace xzero
