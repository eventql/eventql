// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <stx/executor/PosixScheduler.h>
#include <stx/thread/Wakeup.h>
#include <stx/RuntimeError.h>
#include <stx/WallClock.h>
#include <stx/StringUtil.h>
#include <stx/sysconfig.h>
#include <stx/logging.h>
#include <algorithm>
#include <vector>
#include <sstream>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>

namespace stx {

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
      onPostInvokePending_(postInvoke),
      tasks_(),
      timers_(),
      watcherMutex_(),
      watchers_(),
      firstWatcher_(nullptr),
      lastWatcher_(nullptr) {
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
    : PosixScheduler(std::bind(&logAndPass, std::placeholders::_1),
                     WallClock::monotonic(),
                     nullptr,
                     nullptr) {
}

PosixScheduler::~PosixScheduler() {
  TRACE("~dtor");
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

Scheduler::HandleRef PosixScheduler::executeAfter(Duration delay, Task task) {
  return insertIntoTimersList(clock_->get() + delay, task);
}

Scheduler::HandleRef PosixScheduler::executeAt(DateTime when, Task task) {
  return insertIntoTimersList(when, task);
}

Scheduler::HandleRef PosixScheduler::insertIntoTimersList(DateTime dt,
                                                          Task task) {
  RefPtr<Timer> t(new Timer(dt, task));

  auto onCancel = [this, t]() {
    auto i = timers_.end();
    auto e = timers_.begin();

    while (i != e) {
      i--;
      if (*i == t) {
        timers_.erase(i);
        break;
      }
    }
  };
  t->setCancelHandler(onCancel);

  std::lock_guard<std::mutex> lk(lock_);

  auto i = timers_.end();
  auto e = timers_.begin();

  while (i != e) {
    i--;
    const RefPtr<Timer>& current = *i;
    if (current->when >= t->when) {
      i = timers_.insert(i, t);
      return t.as<Handle>();

      // return std::make_shared<Handle>([this, i]() {
      //   std::lock_guard<std::mutex> lk(lock_);
      //   timers_.erase(i);
      // });
    }
  }

  timers_.push_front(t);
  i = timers_.begin();
  return t.as<Handle>();

  // return std::make_shared<Handle>([this, i]() {
  //   std::lock_guard<std::mutex> lk(lock_);
  //   timers_.erase(i);
  // });
}

void PosixScheduler::collectTimeouts(std::list<Task>* result) {
  const DateTime now = clock_->get();

  for (Watcher* w = firstWatcher_; w && w->timeout <= now; ) {
    TRACE("collectTimeouts: timeouting %s", inspect(*w).c_str());
    result->push_back([w] { w->fire(w->onTimeout); });
    switch (w->mode) {
      case Mode::READABLE: readerCount_--; break;
      case Mode::WRITABLE: writerCount_--; break;
    }
    w = unlinkWatcher(w);
  }

  for (;;) {
    if (timers_.empty())
      break;

    const RefPtr<Timer>& job = timers_.front();

    if (job->when > now)
      break;

    result->push_back([job] { job->fire(job->action); });
    timers_.pop_front();
  }
}

void PosixScheduler::linkWatcher(Watcher* w, Watcher* pred) {
  Watcher* succ = pred->next;

  w->prev = pred;
  w->next = succ;

  if (pred)
    pred->next = w;
  else
    firstWatcher_ = w;

  if (succ)
    succ->prev = w;
  else
    lastWatcher_ = w;
}

PosixScheduler::Watcher* PosixScheduler::unlinkWatcher(Watcher* w) {
  Watcher* pred = w->prev;
  Watcher* succ = w->next;

  if (pred)
    pred->next = succ;
  else
    firstWatcher_ = succ;

  if (succ)
    succ->prev = pred;
  else
    lastWatcher_ = pred;

  w->clear();

  return succ;
}

inline PosixScheduler::Watcher* PosixScheduler::getWatcher(int fd) {
  return &watchers_[fd];
}

Scheduler::HandleRef PosixScheduler::executeOnReadable(int fd, Task task, Duration tmo, Task tcb) {
  readerCount_++;
  std::lock_guard<std::mutex> lk(lock_);
  return setupWatcher(fd, Mode::READABLE, task, tmo, tcb);
}

Scheduler::HandleRef PosixScheduler::executeOnWritable(int fd, Task task, Duration tmo, Task tcb) {
  writerCount_++;
  std::lock_guard<std::mutex> lk(lock_);
  return setupWatcher(fd, Mode::WRITABLE, task, tmo, tcb);
}

PosixScheduler::HandleRef PosixScheduler::setupWatcher(
    int fd, Mode mode, Task task,
    Duration tmo, Task tcb) {

  TRACE("setupWatcher(%d, %s, %s)",
      fd, inspect(mode).c_str(), inspect(tmo).c_str());

  DateTime timeout = clock_->get() + tmo;

  if (fd >= watchers_.size()) {
    watchers_.resize(fd + 1);
  }

  Watcher* w = &watchers_[fd];

  if (w->fd >= 0)
    RAISE_STATUS(AlreadyWatchingOnResource);

  w->reset(fd, mode, task, timeout, tcb);

  // inject watcher oredred by timeout ascending
  if (lastWatcher_ != nullptr) {
    Watcher* pred = lastWatcher_;

    while (pred != nullptr) {
      if (w->timeout <= pred->timeout) {
        linkWatcher(w, pred);
        return w; // handle;
      }
    }
    // put in front
    w->next = firstWatcher_;
    firstWatcher_->prev = w;
    firstWatcher_ = w;
    return w; // handle;
  } else {
    firstWatcher_ = lastWatcher_ = w;
  }

  return w; // handle;
}

void PosixScheduler::collectActiveHandles(const fd_set* input,
                                          const fd_set* output,
                                          std::list<Task>* result) {
  Watcher* w = firstWatcher_;

  while (w != nullptr) {
    if (FD_ISSET(w->fd, input)) {
      TRACE("collectActiveHandles: + active fd %d READABLE", w->fd);
      readerCount_--;
      result->push_back(w->onIO);
      w = unlinkWatcher(w);
    }
    else if (FD_ISSET(w->fd, output)) {
      TRACE("collectActiveHandles: + active fd %d WRITABLE", w->fd);
      writerCount_--;
      result->push_back(w->onIO);
      w = unlinkWatcher(w);
    } else {
      TRACE("collectActiveHandles: - skip fd %d", w->fd);
      w = w->next;
    }
  }
}

// FIXME: this is actually so generic, it could be put into Executor API directly
void PosixScheduler::executeOnWakeup(Task task, Wakeup* wakeup, long generation) {
  wakeup->onWakeup(generation, std::bind(&PosixScheduler::execute, this, task));
}

size_t PosixScheduler::timerCount() {
  std::lock_guard<std::mutex> lk(lock_);
  return timers_.size();
}

size_t PosixScheduler::readerCount() {
  return readerCount_.load();
}

size_t PosixScheduler::writerCount() {
  return writerCount_.load();
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
             || firstWatcher_ != nullptr;
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
  int incount = 0;
  int outcount = 0;
  int errcount = 0;

  {
    std::lock_guard<std::mutex> lk(lock_);

    for (const Watcher* w = firstWatcher_; w; w = w->next) {
      if (w->fd < 0)
        continue;

      switch (w->mode) {
        case Mode::READABLE:
          FD_SET(w->fd, &input);
          incount++;
          break;
        case Mode::WRITABLE:
          FD_SET(w->fd, &output);
          outcount++;
          break;
      }
      wmark = std::max(wmark, w->fd);
    }

    const Duration timeout = nextTimeout();
    tv.tv_sec = static_cast<time_t>(timeout.totalSeconds()),
    tv.tv_usec = timeout.microseconds();
  }

  FD_SET(wakeupPipe_[PIPE_READ_END], &input);

  TRACE("runLoopOnce(): select(wmark=%d, in=%d, out=%d, err=%d, tmo=%ds)",
      wmark + 1, incount, outcount, errcount, tv.tv_sec);
  TRACE("runLoopOnce: %s", inspect(*this).c_str());

  int rv;
  do rv = ::select(wmark + 1, &input, &output, &error, &tv);
  while (rv < 0 && errno == EINTR);

  TRACE("runLoopOnce: select returned %i", rv);

  if (rv < 0)
    RAISE_ERRNO(errno);

  if (FD_ISSET(wakeupPipe_[PIPE_READ_END], &input)) {
    bool consumeMore = true;
    while (consumeMore) {
      char buf[sizeof(int) * 128];
      consumeMore = ::read(wakeupPipe_[PIPE_READ_END], buf, sizeof(buf)) > 0;
    }
  }

  std::list<Task> activeTasks;
  {
    std::lock_guard<std::mutex> lk(lock_);

    activeTasks = std::move(tasks_);
    collectActiveHandles(&input, &output, &activeTasks);
    collectTimeouts(&activeTasks);
  }

  safeCall(onPreInvokePending_);
  safeCallEach(activeTasks);
  safeCall(onPostInvokePending_);
}

Duration PosixScheduler::nextTimeout() const {
  if (!tasks_.empty())
    return Duration::Zero;

  const DateTime now = clock_->get();

  const Duration a = !timers_.empty()
                 ? timers_.front()->when - now
                 : Duration::fromSeconds(5);

  const Duration b = firstWatcher_ != nullptr
                 ? firstWatcher_->timeout - now
                 : Duration::fromSeconds(6);

  return std::min(a, b);
}

void PosixScheduler::breakLoop() {
  int dummy = 42;
  ::write(wakeupPipe_[PIPE_WRITE_END], &dummy, sizeof(dummy));
}

void PosixScheduler::waitForReadable(int fd, Duration timeout) {
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

void PosixScheduler::waitForWritable(int fd, Duration timeout) {
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

std::string PosixScheduler::inspectImpl() const {
  std::stringstream sstr;

  sstr << "{";
  sstr << "wakeupPipe:" << wakeupPipe_[PIPE_READ_END]
       << "/" << wakeupPipe_[PIPE_WRITE_END];

  sstr << ", watchers(";
  for (Watcher* w = firstWatcher_; w != nullptr; w = w->next) {
    if (w != firstWatcher_)
      sstr << ", ";
    sstr << inspect(*w);
  }
  sstr << ")"; // watcher-list
  sstr << ", front:" << (firstWatcher_ ? firstWatcher_->fd : -1);
  sstr << ", back:" << (lastWatcher_ ? lastWatcher_->fd : -1);
  sstr << "}"; // scheduler

  return sstr.str();
}

std::string inspect(PosixScheduler::Mode mode) {
  static const std::string modes[] = {
    "READABLE",
    "WRITABLE"
  };
  return modes[static_cast<size_t>(mode)];
}

std::string inspect(const PosixScheduler::Watcher& w) {
  return StringUtil::format("{$0, $1, $2}",
                            w.fd, inspect(w.mode), inspect(w.timeout));
}

std::string inspect(const PosixScheduler& s) {
  return s.inspectImpl();
}

} // namespace stx
