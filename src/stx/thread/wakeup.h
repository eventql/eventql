/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_THREAD_WAKEUP_H
#define _STX_THREAD_WAKEUP_H
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <list>
#include <stx/autoref.h>

namespace stx {

class Wakeup : public RefCounted {
public:
  Wakeup();

  /**
   * Block the current thread and wait for the next wakeup event
   */
  void waitForNextWakeup();
  void waitForFirstWakeup();
  void waitForWakeup(long generation);

  void wakeup();
  void onWakeup(long generation, std::function<void()> callback);

  long generation() const;

protected:
  std::mutex mutex_;
  std::condition_variable condvar_;
  std::atomic<long> gen_;
  std::list<std::function<void()>> callbacks_;
};

}
#endif
