/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_TSDB_COMPACTIONWORKER_H
#define _FNORD_TSDB_COMPACTIONWORKER_H
#include <thread>
#include <stx/stdtypes.h>
#include <tsdb/TSDBNodeRef.h>

using namespace stx;

namespace tsdb {

class CompactionWorker : public RefCounted {
public:

  CompactionWorker(TSDBNodeRef* node);
  void start();
  void stop();

protected:
  void run();

  thread::CoalescingDelayedQueue<Partition>* queue_;
  std::atomic<bool> running_;
  std::thread thread_;
};

} // namespace tsdb

#endif
