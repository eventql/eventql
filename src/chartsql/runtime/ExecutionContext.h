/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord/stdtypes.h>
#include <fnord/autoref.h>
#include <chartsql/svalue.h>

using namespace fnord;

namespace csql {

struct ExecutionStatus {
  ExecutionStatus();

  size_t num_subtasks_total;
  size_t num_subtasks_completed;

  String toString() const;
  double progress() const;
};


class ExecutionContext : public RefCounted {
public:

  ExecutionContext();

  void updateStatus(Function<void (ExecutionStatus* status)> fn);
  void onStatusChange(Function<void ()> fn);
  ExecutionStatus status() const;

  void cancel();
  bool isCancelled() const;
  void onCancel(Function<void ()> fn);

protected:
  ExecutionStatus status_;
  mutable std::mutex mutex_;
  Function<void ()> on_status_change_;
  Function<void ()> on_cancel_;
  bool cancelled_;
};

}
