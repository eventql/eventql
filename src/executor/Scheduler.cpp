// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/executor/Scheduler.h>

namespace xzero {

Scheduler::Handle::Handle(Task onFire, std::function<void(Handle*)> onCancel)
    : mutex_(),
      onFire_(onFire),
      onCancel_(onCancel),
      isCancelled_(false) {
}

void Scheduler::Handle::cancel() {
  std::lock_guard<std::mutex> lk(mutex_);

  isCancelled_.store(true);

  if (onCancel_) {
    auto cancelThat = std::move(onCancel_);
    cancelThat(this);
  }
}

void Scheduler::Handle::fire() {
  std::lock_guard<std::mutex> lk(mutex_);

  if (!isCancelled_.load()) {
    onFire_();
  }
}

}  // namespace xzero
