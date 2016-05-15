/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#ifndef _libstx_THREAD_DELAYEDQUEUE_H
#define _libstx_THREAD_DELAYEDQUEUE_H
#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include "eventql/util/option.h"
#include "eventql/util/UnixTime.h"

namespace util {
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
