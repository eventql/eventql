// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/support/libev/LibevScheduler.h>

namespace xzero {
namespace support {


/* TODO
 * - make use of pending_ member var to implement pending-task cancellation.
 */

struct LibevScheduler::TaskInfo {
  Task task;
  ev::timer timer;

  TaskInfo(ev::loop_ref loop, TimeSpan delay, Task&& t);
  ~TaskInfo();

  void fire(ev::timer&, int);
};

LibevScheduler::TaskInfo::TaskInfo(ev::loop_ref l, TimeSpan d, Task&& t)
    : task(t), timer(l) {

  timer.set<LibevScheduler::TaskInfo, &LibevScheduler::TaskInfo::fire>(this);
  timer.start(d.value(), 0);
}

LibevScheduler::TaskInfo::~TaskInfo() {
  //.
}

void LibevScheduler::TaskInfo::fire(ev::timer&, int) {
  try {
    if (task) {
      task();
    }
  } catch (...) {
    delete this;
    throw;
  }
  delete this;
}

LibevScheduler::LibevScheduler(
    ev::loop_ref loop,
    std::function<void(const std::exception&)>&& eh)
    : Scheduler(std::move(eh)),
      loop_(loop),
      evWakeup_(loop_),
      pending_() {
  evWakeup_.set<LibevScheduler, &LibevScheduler::onWakeup>(this);
  evWakeup_.start();
}

LibevScheduler::~LibevScheduler() {
  //.
}

void LibevScheduler::execute(Task&& task) {
  new TaskInfo(loop_, TimeSpan::Zero,
               std::bind(&LibevScheduler::safeCall, this, task));
  wakeup();
}

void LibevScheduler::schedule(TimeSpan delay, Task&& task) {
  new TaskInfo(loop_, delay, std::bind(&LibevScheduler::safeCall, this, task));
  wakeup();
}

void LibevScheduler::wakeup() {
  evWakeup_.send();
  loop_.break_loop(ev::ALL);
}

void LibevScheduler::onWakeup(ev::async&, int) {
  loop_.break_loop(ev::ALL);
}

size_t LibevScheduler::maxConcurrency() const XZERO_NOEXCEPT {
  return 1;
}

std::string LibevScheduler::toString() const {
  char buf[128];
  snprintf(buf, sizeof(buf), "LibevScheduler@%p", this);
  return buf;
}

}  // namespace support
}  // namespace xzero
