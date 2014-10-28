#include <xzero/executor/DirectLoopExecutor.h>
#include <xzero/sysconfig.h>
#include <system_error>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

namespace xzero {

DirectLoopExecutor::DirectLoopExecutor()
    : pending_(),
      isCancelled_(false),
      pipe_{-1, -1} {

#if defined(HAVE_PIPE2)
  ssize_t rv = pipe2(pipe_, O_NONBLOCK | O_CLOEXEC);
  if (rv < 0)
    throw std::system_error(errno, std::system_category());
#else
  if (pipe(pipe_) < 0)
    throw std::system_error(errno, std::system_category());

  for (int i = 0; i < 2; ++i) {
    if (fcntl(pipe_[i], F_SETFL, O_NONBLOCK) < 0)
      throw std::system_error(errno, std::system_category());

    if (fcntl(pipe_[i], F_SETFD, FD_CLOEXEC) < 0)
      throw std::system_error(errno, std::system_category());
  }
#endif
}

DirectLoopExecutor::~DirectLoopExecutor() {
  cancel();
}

void DirectLoopExecutor::run() {
  for (;;) {
    waitForEvent();
    if (isCancelled_)
      return;

    for (;;) {
      if (!tryRunOne())
        break;

      if (isCancelled_)
        return;
    }
  }
}

bool DirectLoopExecutor::tryRunAllPending() {
  unsigned i = 0;
  for (;;) {
    if (!tryRunOne())
      return i > 0;

    if (isCancelled_)
      return i > 0;
  }
}

bool DirectLoopExecutor::tryRunOne() {
  if (pending_.empty())
    return false;

  pending_.front()();
  pending_.pop_front();
  return true;
}

void DirectLoopExecutor::cancel() {
  isCancelled_ = true;
  wakeup();
}

void DirectLoopExecutor::execute(Task&& task) {
  pending_.emplace_back(std::forward<Task>(task));

  wakeup();
}

size_t DirectLoopExecutor::maxConcurrency() const noexcept {
  return 1;
}

bool DirectLoopExecutor::waitForEvent() {
  fd_set rs;
  FD_ZERO(&rs);
  FD_SET(pipe_[0], &rs);
  select(pipe_[0] + 1, &rs, nullptr, nullptr, nullptr);

  uint64_t buf[64];
  ssize_t rv = ::read(pipe_[0], buf, sizeof(buf));
  return rv > 0;
}

void DirectLoopExecutor::wakeup() {
  uint64_t val = 1;
  ::write(pipe_[1], &val, sizeof(val));
}

std::string DirectLoopExecutor::toString() const {
  char buf[64];
  snprintf(buf, sizeof(buf), "DirectLoopExecutor@%p", this);
  return buf;
}

} // namespace xzero
