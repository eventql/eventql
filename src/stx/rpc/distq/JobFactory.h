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
#include "stx/rpc/distq/Job.h"

using namespace stx;

namespace stx {
namespace distq {

class JobFactory {
public:

  RefPtr<Job> getJob(
      const String& job_name,
      const Buffer& params) const;

  /**
   * Register job
   */
  template <class ParamType, class ResultType>
  void registerJob(
      const String& job_name,
      Function<void (RefPtr<TypedJob<ParamType, ResultType>> fn);

protected:

  struct JobFactoryMethods {
    //Function<void (RefPtr<QueryContext> ctx)> call_;
    //Function<RefPtr<QueryContext> (InputStream* is))> build_;
  };

  mutable std::mutex mutex_;
  HashMap<String, JobFactoryMethods> jobs_;
};

} // namespace distq
} // namespace stx

