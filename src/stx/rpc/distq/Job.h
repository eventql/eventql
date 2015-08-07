/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "stx/stdtypes.h"
#include "stx/duration.h"
#include "stx/exception.h"

using namespace stx;

namespace stx {
namespace distq {

class Job {
public:

  Job(const String& job_name);

  void cancel();
  bool isCancelled() const;
  void onCancel(Function<void ()> fn);

  void wait() const;
  bool waitFor(const Duration& timeout) const;

  HashMap<String, double> getCounters() const;
  double getCounter(const String& counter) const;

  double getProgress() const;
  void onProgress(Function<void (double progress)> fn);

  void sendError(const String& error);
  void sendProgress(double progress);
  void incrCounter(const String& counter, double value);

  template <typename ResultType>
  void onResult(Function<void (const ResultType& res)> fn);

  template <typename ResultType>
  void sendResult(const ResultType& res);

private:
  String name_;
  bool cancelled_;
  bool ready_;
  bool error_;
  std::function<void ()> on_ready_;
  std::function<void (String)> on_error_;
  std::function<void ()> on_cancel_;
  HashMap<String, double> counters_;
  mutable std::mutex mutex_;
  mutable std::condition_variable cv_;
};


} // namespace distq
} // namespace stx

