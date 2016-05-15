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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/thread/taskscheduler.h>
#include <eventql/sql/svalue.h>

using namespace stx;

namespace csql {

struct ExecutionStatus {
  ExecutionStatus();

  std::atomic<size_t> num_subtasks_total;
  std::atomic<size_t> num_subtasks_completed;
  std::atomic<size_t> num_rows_scanned;

  String toString() const;
  double progress() const;
};


class ExecutionContext : public RefCounted {
public:

  ExecutionContext(
      TaskScheduler* sched,
      size_t max_concurrent_tasks = 32);

  void onStatusChange(Function<void (const ExecutionStatus& status)> fn);

  void cancel();
  bool isCancelled() const;
  void onCancel(Function<void ()> fn);

  void incrNumSubtasksTotal(size_t n);
  void incrNumSubtasksCompleted(size_t n);
  void incrNumRowsScanned(size_t n);

  void runAsync(Function<void ()> fn);

  Option<String> cacheDir() const;
  void setCacheDir(const String& cachedir);

protected:

  void statusChanged();

  TaskScheduler* sched_;
  size_t max_concurrent_tasks_;
  Option<String> cachedir_;

  ExecutionStatus status_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  Function<void (const ExecutionStatus& status)> on_status_change_;
  Function<void ()> on_cancel_;
  bool cancelled_;
  size_t running_tasks_;
};

}
