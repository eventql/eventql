// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/support/libev/LibevScheduler.h>
#include <xzero/io/SelectionKey.h>
#include <xzero/io/Selectable.h>

namespace xzero {
namespace support {


/* TODO
 * - make use of pending_ member var to implement pending-task cancellation.
 */

#if 0
/**
 * Wrapper around @c ev::io.
 */
class LibevIO : public SelectionKey { // {{{
 public:
  LibevIO(LibevScheduler* selector, Selectable* selectable, unsigned ops);
  ~LibevIO();
  int interest() const XZERO_NOEXCEPT override;
  void change(int ops) override;
  void io(ev::io&, int revents);

 private:
  ev::io io_;
  Selectable* selectable_;
  int interest_;
};

LibevIO::LibevIO(LibevScheduler* selector, Selectable* selectable, unsigned ops)
    : SelectionKey(selector),
      io_(selector->loop()),
      selectable_(selectable),
      interest_(0) {

  assert(selector != nullptr);
  assert(selectable != nullptr);
  assert(selectable->handle() >= 0);
  assert((ops & Selectable::READ) || (ops & Selectable::WRITE));

  TRACE("LibevIO: fd=%d, ops=%u", selectable->handle(), ops);

  io_.set<LibevIO, &LibevIO::io>(this);
  change(ops);

  io_.start();
}

LibevIO::~LibevIO() {
  TRACE("~LibevIO: fd=%d", io_.fd);
}

int LibevIO::interest() const XZERO_NOEXCEPT override {
  return interest_;
}

void LibevIO::change(int ops) override {
  unsigned flags = 0;
  if (ops & Selectable::READ)
    flags |= ev::READ;

  if (ops & Selectable::WRITE)
    flags |= ev::WRITE;

  TRACE("LibevIO.change: fd=%d, ops=%d", selectable_->handle(), ops);

  if (interest_ != flags) {
    BUG_ON(selectable_->handle() < 0);
    io_.set(selectable_->handle(), flags);
    interest_ = flags;
  }
}

void LibevIO::io(ev::io&, int revents) {
  unsigned flags = 0;

  if (revents & ev::READ)
    flags |= Selectable::READ;

  if (revents & ev::WRITE)
    flags |= Selectable::WRITE;

  setActivity(flags);

  try {
    selectable_->onSelectable();
  } catch (const std::exception& e) {
    static_cast<LibevScheduler*>(selector())->logError(e);
  } catch (...) {
    static_cast<LibevScheduler*>(selector())->logError(RUNTIME_ERROR(
          "Unknown exception caught."));
  }
}
// }}}
#endif
struct LibevScheduler::TaskInfo { // {{{
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
// }}}

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
  //loop_.break_loop(ev::ONE);
}

void LibevScheduler::onWakeup(ev::async&, int) {
  loop_.break_loop(ev::ONE);
}

size_t LibevScheduler::maxConcurrency() const XZERO_NOEXCEPT {
  return 1;
}

std::string LibevScheduler::toString() const {
  char buf[128];
  snprintf(buf, sizeof(buf), "LibevScheduler@%p", this);
  return buf;
}

// std::unique_ptr<SelectionKey> LibevScheduler::createSelectable(
//     Selectable* handle, unsigned ops) {
//   return std::unique_ptr<SelectionKey>(new LibevIO(this, handle, ops));
// }

}  // namespace support
}  // namespace xzero
