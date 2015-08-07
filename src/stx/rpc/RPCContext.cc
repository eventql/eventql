/*
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/stdtypes.h"
#include "stx/rpc/RPCContext.h"

namespace stx {
namespace rpc {

JobContext::JobContext(Job* job) : job_(job) {}

bool JobContext::isCancelled() const {
  std::unique_lock<std::mutex> lk(job_->mutex_);
  return job_->error_ != nullptr;
}

void JobContext::onCancel(Function<void ()> fn) {
  std::unique_lock<std::mutex> lk(job_->mutex_);

  if (job_->error_ != nullptr) {
    lk.unlock();
    fn();
  } else {
    job_->on_cancel_ = fn;
  }
}


} // namespace rpc
} // namespace stx

