// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/IdleTimeout.h>
#include <xzero/WallClock.h>
#include <xzero/logging/LogSource.h>
#include <xzero/executor/Scheduler.h>
#include <assert.h>

namespace xzero {

static LogSource idleTimeoutLogger("IdleTimeout");
#define ERROR(msg...) do { idleTimeoutLogger.error(msg); } while (0)
#ifndef NDEBUG
#define TRACE(msg...) do { idleTimeoutLogger.trace(msg); } while (0)
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

  TimeSpan deltaTimeout = timeout_ - (clock_->get() - fired_);
  scheduler_->schedule(deltaTimeout, std::bind(&IdleTimeout::onFired, this));
}

void IdleTimeout::schedule() {
  fired_ = clock_->get();
  scheduler_->schedule(timeout_, std::bind(&IdleTimeout::onFired, this));
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

} // namespace xzero
