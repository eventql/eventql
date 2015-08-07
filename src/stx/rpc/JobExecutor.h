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
#include "stx/rpc/Job.h"

namespace stx {
namespace rpc {

class JobExecutor {
public:

  virtual ~JobExecutor() {}

  virtual RefPtr<Job> getJob(
      const String& method,
      const Serializable& params) = 0;

};


class LocalExecutor : public JobExecutor {
public:

  RefPtr<Job> getJob(
      const String& method,
      const Serializable& params) override;

  template <class ParamType>
  void registerJob(
      const String& job_name,
      Function<void (const ParamType& params, JobContext* ctx)> fn);

protected:

  struct JobFactoryMethods {
    Function<RefPtr<Job> (const Serializable& params)> ctor_ref;
    Function<RefPtr<Job> (InputStream* is)> ctor_io;
  };

  mutable std::mutex mutex_;
  HashMap<String, JobFactoryMethods> jobs_;
};

template <class ParamType>
void LocalExecutor::registerJob(
    const String& job_name,
    Function<void (const ParamType& params, JobContext* ctx)> fn) {
  JobFactoryMethods fmethods;

  fmethods.ctor_ref = [fn] (const Serializable& any_params) -> RefPtr<Job> {
    const auto& params = dynamic_cast<const ParamType&>(any_params);
    return new Job([fn, params] (JobContext* ctx) {
      fn(params, ctx);
    });
  };

  std::unique_lock<std::mutex> lk(mutex_);
  jobs_.emplace(job_name, fmethods);
}


} // namespace rpc
} // namespace stx

