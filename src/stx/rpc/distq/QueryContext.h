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
#include "stx/stdtypes.h"

using namespace stx;

namespace stx {
namespace distq {

class QueryContext {
public:

  QueryContext(
      const String& method);

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

protected:
  mutable std::mutex mutex_;
  String method_;
  HashMap<String, double> counters_;
};

template <typename ParamType, typename ResultType>
class TypedQueryContext : public QueryContext {
public:

  TypedQueryContext(
      const String& method,
      const ParamType& params);

  const ParamType& params() const;

  void onResult(Function<void (const ResultType& res)> fn);
  void sendResult(const ResultType& res);

protected:
  ParamType params_;
};


} // namespace distq
} // namespace stx

