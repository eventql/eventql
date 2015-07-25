// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/IdleTimeout.h>
#include <cortex-base/WallClock.h>
#include <cortex-base/logging.h>
#include <cortex-base/executor/Scheduler.h>
#include <assert.h>

namespace cortex {

#define ERROR(msg...) do { logError("IdleTimeout", msg); } while (0)
#ifndef NDEBUG
#define TRACE(msg...) do { logTrace("IdleTimeout", msg); } while (0)
#else
#define TRACE(msg...) do {} while (0)
#endif

IdleTimeout::IdleTimeout(WallClock* clock, Scheduler* scheduler) :
  clock_(clock),
  scheduler_(scheduler),
  timeout_(TimeSpan::Zero),
  fired_(),
  active_(false),
  onTimeout_() {
}

IdleTimeout::~IdleTimeout() {
}

void IdleTimeout::setTimeout(TimeSpan value) {
  timeout_ = value;
}

TimeSpan IdleTimeout::timeout() const {
  return timeout_;
}

void IdleTimeout::setCallback(std::function<void()>&& cb) {
  onTimeout_ = std::move(cb);
}

void IdleTimeout::clearCallback() {
  onTimeout_ = decltype(onTimeout_)();
}

void IdleTimeout::touch() {
  if (isActive()) {
    schedule();
  }
}

void IdleTimeout::activate() {
  assert(onTimeout_ && "No timeout callback defined");
  if (!active_) {
    active_ = true;
    schedule();
  }
}

void IdleTimeout::deactivate() {
  active_ = false;
}

TimeSpan IdleTimeout::elapsed() const {
  if (isActive()) {
    return clock_->get() - fired_;
  } else {
    return TimeSpan::Zero;
  }
}

void IdleTimeout::reschedule() {
  assert(isActive());

  handle_->cancel();

  TimeSpan deltaTimeout = timeout_ - (clock_->get() - fired_);
  handle_ = scheduler_->executeAfter(deltaTimeout,
                                     std::bind(&IdleTimeout::onFired, this));
}

void IdleTimeout::schedule() {
  fired_ = clock_->get();

  if (handle_)
    handle_->cancel();

  handle_ = scheduler_->executeAfter(timeout_,
                                     std::bind(&IdleTimeout::onFired, this));
}

void IdleTimeout::onFired() {
  TRACE("IdleTimeout(%p).onFired: active=%d", this, active_);
  if (!active_) {
    return;
  }

  if (elapsed() >= timeout_) {
    active_ = false;
    onTimeout_();
  } else if (isActive()) {
    reschedule();
  }
}

bool IdleTimeout::isActive() const {
  return active_;
}

} // namespace cortex
