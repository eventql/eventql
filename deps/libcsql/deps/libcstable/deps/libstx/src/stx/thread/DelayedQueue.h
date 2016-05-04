/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_THREAD_DELAYEDQUEUE_H
#define _libstx_THREAD_DELAYEDQUEUE_H
#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include "stx/option.h"
#include "stx/UnixTime.h"

namespace stx {
namespace thread {

/**
 * A queue is threadsafe
 */
template <typename T>
class DelayedQueue {
public:

  DelayedQueue(size_t max_size = -1);

  void insert(const T& job, UnixTime when, bool block = false);
  Option<T> interruptiblePop();

  size_t length() const;
  void wakeup();

protected:
  size_t max_size_;
  size_t length_;

  std::multiset<
      Pair<uint64_t, T>,
      Function<bool (
          const Pair<uint64_t, T>&,
          const Pair<uint64_t, T>&)>> queue_;

  mutable std::mutex mutex_;
  std::condition_variable wakeup_;
};

}
}

#include "DelayedQueue_impl.h"
#endif
