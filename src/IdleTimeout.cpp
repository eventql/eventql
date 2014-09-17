#include <xzero/IdleTimeout.h>
#include <xzero/WallClock.h>
#include <xzero/executor/Scheduler.h>
#include <assert.h>

namespace xzero {

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

TimeSpan IdleTimeout::getTimeout() const {
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
  active_ = true;
  schedule();
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
