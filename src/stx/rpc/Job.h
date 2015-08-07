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
#include "stx/autoref.h"
#include "stx/duration.h"
#include "stx/exception.h"
#include "stx/Serializable.h"
#include "stx/io/inputstream.h"

namespace stx {
namespace rpc {
class JobContext;

class Job : public RefCounted {
  friend class JobContext;
public:

  Job(Function<void (JobContext* ctx)> call_fn);

  void run();
  void cancel();

  void wait() const;
  bool waitFor(const Duration& timeout) const;

  HashMap<String, double> getCounters() const;
  double getCounter(const String& counter) const;

  double getProgress() const;
  void onProgress(Function<void (double progress)> fn);

  template <typename ResultType>
  void onResult(Function<void (const ResultType& res)> fn);

protected:
  Function<void (JobContext* ctx)> call_fn_;
  bool cancelled_;
  bool ready_;
  bool error_;
  Function<void (const Serializable& result)> on_result_;
  Function<void ()> on_ready_;
  Function<void (String)> on_error_;
  Function<void ()> on_cancel_;
  HashMap<String, double> counters_;
  mutable std::mutex mutex_;
  mutable std::condition_variable cv_;
};

class JobContext {
public:

  JobContext(Job* job);

  bool isCancelled() const;
  void onCancel(Function<void ()> fn);

  template <typename ResultType>
  void sendResult(const ResultType& res);

  void sendError(const String& error);
  void sendProgress(double progress);

  void incrCounter(const String& counter, double value);

protected:
  Job* job_;
};

} // namespace rpc
} // namespace stx

