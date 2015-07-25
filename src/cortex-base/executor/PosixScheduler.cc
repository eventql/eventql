// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/executor/PosixScheduler.h>
#include <cortex-base/thread/Wakeup.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/WallClock.h>
#include <cortex-base/StringUtil.h>
#include <cortex-base/sysconfig.h>
#include <cortex-base/logging.h>
#include <algorithm>
#include <vector>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>

namespace cortex {

#define PIPE_READ_END  0
#define PIPE_WRITE_END 1

#define ERROR(msg...) logError("PosixScheduler", msg)

#ifndef NDEBUG
#define TRACE(msg...) logTrace("PosixScheduler", msg)
#else
#define TRACE(msg...) do {} while (0)
#endif

PosixScheduler::PosixScheduler(
    std::function<void(const std::exception&)> errorLogger,
    WallClock* clock,
    std::function<void()> preInvoke,
    std::function<void()> postInvoke)
    : Scheduler(errorLogger),
      clock_(clock ? clock : WallClock::monotonic()),
      lock_(),
      wakeupPipe_(),
      onPreInvokePending_(preInvoke),
      onPostInvokePending_(postInvoke) {
  if (pipe(wakeupPipe_) < 0) {
    RAISE_ERRNO(errno);
  }
  fcntl(wakeupPipe_[0], F_SETFL, O_NONBLOCK);
  fcntl(wakeupPipe_[1], F_SETFL, O_NONBLOCK);

  TRACE("ctor: wakeupPipe {read=%d, write=%d}, clock=@%p",
      wakeupPipe_[PIPE_READ_END],
      wakeupPipe_[PIPE_WRITE_END],
      clock_);
}

PosixScheduler::PosixScheduler(
    std::function<void(const std::exception&)> errorLogger,
    WallClock* clock)
    : PosixScheduler(errorLogger, clock, nullptr, nullptr) {
}

PosixScheduler::PosixScheduler()
    : PosixScheduler(nullptr, nullptr, nullptr, nullptr) {
}

PosixScheduler::~PosixScheduler() {
  TRACE("~PosixScheduler");
  ::close(wakeupPipe_[PIPE_READ_END]);
  ::close(wakeupPipe_[PIPE_WRITE_END]);
}

void PosixScheduler::execute(Task task) {
  {
    std::lock_guard<std::mutex> lk(lock_);
    tasks_.emplace_back(std::move(task));
  }
  breakLoop();
}

std::string PosixScheduler::toString() const {
  return StringUtil::format("PosixScheduler: wakeupPipe{$0, $1}",
      wakeupPipe_[PIPE_READ_END],
      wakeupPipe_[PIPE_WRITE_END]);
}

Scheduler::HandleRef PosixScheduler::executeAfter(TimeSpan delay, Task task) {
  auto onCancel = [this](Handle* handle) {
    removeFromTimersList(handle);
  };

  return insertIntoTimersList(clock_->get() + delay,
                              std::make_shared<Handle>(task, onCancel));
}

Scheduler::HandleRef PosixScheduler::executeAt(DateTime when, Task task) {
  return insertIntoTimersList(
      when,
      std::make_shared<Handle>(task,
                               std::bind(&PosixScheduler::removeFromTimersList,
                                         this,
                                         std::placeholders::_1)));
}

Scheduler::HandleRef PosixScheduler::insertIntoTimersList(DateTime dt,
                                                           HandleRef handle) {
  Timer t = { dt, handle };

  std::lock_guard<std::mutex> lk(lock_);

  auto i = timers_.end();
  auto e = timers_.begin();

  while (i != e) {
    i--;
    const Timer& current = *i;
    if (current.when >= t.when) {
      timers_.insert(i, t);
      return handle;
    }
  }

  timers_.push_front(t);
  return handle;
}

void PosixScheduler::removeFromTimersList(Handle* handle) {
  std::lock_guard<std::mutex> lk(lock_);

  for (auto i = timers_.begin(), e = timers_.end(); i != e; ++i) {
    if (i->handle.get() == handle) {
      timers_.erase(i);
      break;
    }
  }
}

void PosixScheduler::collectTimeouts(std::vector<HandleRef>* result) {
  const DateTime now = clock_->get();

  for (;;) {
    if (timers_.empty())
      break;

    const Timer& job = timers_.front();

    if (job.when > now)
      break;

    result->push_back(job.handle);
    timers_.pop_front();
  }
}

inline Scheduler::HandleRef registerInterest(
    std::mutex* registryLock,
    std::list<std::pair<int, Scheduler::HandleRef>>* registry,
    int fd,
    Executor::Task task) {

  auto onCancel = [=](Scheduler::Handle* h) {
    std::lock_guard<std::mutex> lk(*registryLock);
    for (auto i: *registry) {
      if (i.second.get() == h) {
        return registry->remove(i);
      }
    }
  };

  std::lock_guard<std::mutex> lk(*registryLock);
  auto handle = std::make_shared<Scheduler::Handle>(task, onCancel);
  registry->push_back(std::make_pair(fd, handle));

  return handle;
}

Scheduler::HandleRef PosixScheduler::executeOnReadable(int fd, Task task) {
  return registerInterest(&lock_, &readers_, fd, task);
}

Scheduler::HandleRef PosixScheduler::executeOnWritable(int fd, Task task) {
  return registerInterest(&lock_, &writers_, fd, task);
}

// FIXME: this is actually so generic, it could be put into Executor API directly
void PosixScheduler::executeOnWakeup(Task task, Wakeup* wakeup, long generation) {
  wakeup->onWakeup(
      generation,
      std::bind(&PosixScheduler::execute, this, task));
}

inline void collectActiveHandles(
    std::list<std::pair<int, Scheduler::HandleRef>>* interests,
    fd_set* fdset,
    std::vector<Scheduler::HandleRef>* result) {

  auto i = interests->begin();
  auto e = interests->end();

  while (i != e) {
    if (FD_ISSET(i->first, fdset)) {
      result->push_back(i->second);
      i = interests->erase(i);
    } else {
      i++;
    }
  }
}

size_t PosixScheduler::timerCount() {
  std::lock_guard<std::mutex> lk(lock_);
  return timers_.size();
}

size_t PosixScheduler::readerCount() {
  std::lock_guard<std::mutex> lk(lock_);
  return readers_.size();
}

size_t PosixScheduler::writerCount() {
  std::lock_guard<std::mutex> lk(lock_);
  return writers_.size();
}

size_t PosixScheduler::taskCount() {
  std::lock_guard<std::mutex> lk(lock_);
  return tasks_.size();
}

void PosixScheduler::runLoop() {
  for (;;) {
    lock_.lock();
    bool cont = !tasks_.empty()
             || !timers_.empty()
             || !readers_.empty()
             || !writers_.empty();
    lock_.unlock();

    if (!cont)
      break;

    runLoopOnce();
  }
}

void PosixScheduler::runLoopOnce() {
  fd_set input, output, error;
  FD_ZERO(&input);
  FD_ZERO(&output);
  FD_ZERO(&error);

  int wmark = 0;
  timeval tv;

  {
    std::lock_guard<std::mutex> lk(lock_);

    for (auto i: readers_) {
      FD_SET(i.first, &input);
      if (i.first > wmark) {
        wmark = i.first;
      }
    }

    for (auto i: writers_) {
      FD_SET(i.first, &output);
      if (i.first > wmark) {
        wmark = i.first;
      }
    }

    const TimeSpan nextTimeout = !tasks_.empty()
                               ? TimeSpan::Zero
                               : !timers_.empty()
                                 ? timers_.front().when - clock_->get()
                                 : TimeSpan::fromSeconds(4);

    tv.tv_sec = static_cast<time_t>(nextTimeout.totalSeconds()),
    tv.tv_usec = nextTimeout.microseconds();
  }

  FD_SET(wakeupPipe_[PIPE_READ_END], &input);

  int rv;
  do rv = ::select(wmark + 1, &input, &output, &error, &tv);
  while (rv < 0 && errno == EINTR);

  if (rv < 0)
    RAISE_ERRNO(errno);

  if (FD_ISSET(wakeupPipe_[PIPE_READ_END], &input)) {
    bool consumeMore = true;
    while (consumeMore) {
      char buf[sizeof(int) * 128];
      consumeMore = ::read(wakeupPipe_[PIPE_READ_END], buf, sizeof(buf)) > 0;
    }
  }

  std::vector<HandleRef> activeHandles;
  activeHandles.reserve(rv);

  std::deque<Task> activeTasks;
  {
    std::lock_guard<std::mutex> lk(lock_);

    collectTimeouts(&activeHandles);
    collectActiveHandles(&readers_, &input, &activeHandles);
    collectActiveHandles(&writers_, &output, &activeHandles);
    activeTasks = std::move(tasks_);
  }

  safeCall(onPreInvokePending_);
  safeCallEach(activeHandles);
  safeCallEach(activeTasks);
  safeCall(onPostInvokePending_);
}

void PosixScheduler::breakLoop() {
  int dummy = 42;
  ::write(wakeupPipe_[PIPE_WRITE_END], &dummy, sizeof(dummy));
}

void PosixScheduler::waitForReadable(int fd, TimeSpan timeout) {
  fd_set input, output;

  FD_ZERO(&input);
  FD_ZERO(&output);
  FD_SET(fd, &input);

  struct timeval tv;
  tv.tv_sec = timeout.totalSeconds();
  tv.tv_usec = timeout.totalMicroseconds();

  int res = select(fd + 1, &input, &output, &input, &tv);

  if (res == 0) {
    RAISE(IOError, "unexpected timeout while select()ing");
  }

  if (res == -1) {
    RAISE_ERRNO(errno);
  }
}

void PosixScheduler::waitForReadable(int fd) {
  fd_set input, output;

  FD_ZERO(&input);
  FD_ZERO(&output);
  FD_SET(fd, &input);

  int res = select(fd + 1, &input, &output, &input, nullptr);

  if (res == 0) {
    RAISE(IOError, "unexpected timeout while select()ing");
  }

  if (res == -1) {
    RAISE_ERRNO(errno);
  }
}

void PosixScheduler::waitForWritable(int fd, TimeSpan timeout) {
  fd_set input;
  FD_ZERO(&input);

  fd_set output;
  FD_ZERO(&output);
  FD_SET(fd, &output);

  struct timeval tv;
  tv.tv_sec = timeout.totalSeconds();
  tv.tv_usec = timeout.totalMicroseconds();

  int res = select(fd + 1, &input, &output, &input, &tv);

  if (res == 0) {
    RAISE(IOError, "unexpected timeout while select()ing");
  }

  if (res == -1) {
    RAISE_ERRNO(errno);
  }
}

void PosixScheduler::waitForWritable(int fd) {
  fd_set input;
  FD_ZERO(&input);

  fd_set output;
  FD_ZERO(&output);
  FD_SET(fd, &output);

  int res = select(fd + 1, &input, &output, &input, nullptr);

  if (res == 0) {
    RAISE(IOError, "unexpected timeout while select()ing");
  }

  if (res == -1) {
    RAISE_ERRNO(errno);
  }
}

} // namespace cortex
