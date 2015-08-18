/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_THREAD_COALSECINGDELAYEDQUEUE_H
#define _libstx_THREAD_COALSECINGDELAYEDQUEUE_H
#include <atomic>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include <list>
#include "stx/option.h"

namespace stx {
namespace thread {

/**
 * A queue is threadsafe
 */
template <typename T>
class CoalescingDelayedQueue {
public:

  CoalescingDelayedQueue(size_t max_size = -1);

  void insert(RefPtr<T> job, UnixTime when, bool block = false);
  Option<RefPtr<T>> interruptiblePop();

  size_t length() const;
  void wakeup();

protected:
  size_t max_size_;
  size_t length_;

  std::multiset<
      Pair<uint64_t, RefPtr<T>>,
      Function<bool (
          const Pair<uint64_t, RefPtr<T>>&,
          const Pair<uint64_t, RefPtr<T>>&)>> queue_;

  std::unordered_map<T*, uint64_t> map_;

  mutable std::mutex mutex_;
  std::condition_variable wakeup_;
};

}
}

#include "CoalescingDelayedQueue_impl.h"
#endif
