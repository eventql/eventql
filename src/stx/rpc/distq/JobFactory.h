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
#include "stx/Serializable.h"

using namespace stx;

namespace stx {
namespace distq {

class JobFactory {
public:

  RefPtr<Job> getJob(
      const String& job_name,
      const Serializable& params) const;

  /**
   * Register job
   */
  template <class ParamType>
  void registerJob(const String& job_name);

protected:

  struct JobFactoryMethods {
    Function<RefPtr<Job> (const Serializable& params)> build_local_;
    Function<RefPtr<Job> (InputStream* is)> build_remote_;
  };

  mutable std::mutex mutex_;
  HashMap<String, JobFactoryMethods> jobs_;
};

} // namespace distq
} // namespace stx

