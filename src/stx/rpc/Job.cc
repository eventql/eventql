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
#include "stx/rpc/Job.h"

namespace stx {
namespace rpc {

Job::Job(
    Function<void (JobContext* ctx)> call_fn) :
    call_fn_(call_fn),
    ready_(false),
    error_(nullptr),
    running_(false) {}

void Job::run() {
  {
    std::unique_lock<std::mutex> lk(mutex_);
    if (ready_ || running_) {
      RAISE(kRuntimeError, "refusing to run a finished/running job");
    }

    running_ = true;
  }

  JobContext ctx(this);
  try {
    call_fn_(&ctx);
  } catch (const StandardException& e) {
    returnError(e);
    return;
  }

  return returnSuccess();
}

void Job::returnSuccess() {
  {
    std::unique_lock<std::mutex> lk(mutex_);
    ready_ = true;
  }

  cv_.notify_all();

  if (on_ready_) {
    on_ready_();
  }

  on_ready_ = nullptr;
  on_cancel_ = nullptr;
  on_event_.clear();
}

void Job::returnError(const StandardException& e) {
  try {
    auto rte = dynamic_cast<const stx::Exception&>(e);
    returnError(rte.getType(), rte.getMessage());
  } catch (const std::exception& cast_error) {
    returnError(kRuntimeError, e.what());
  }
}

void Job::returnError(ExceptionType error_type, const String& message) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (ready_) {
    RAISE(kRuntimeError, "refusing to send an error to a finished job");
  }

  error_ = error_type;
  error_message_ = message;
  ready_ = true;
  lk.unlock();

  cv_.notify_all();

  if (on_ready_) {
    on_ready_();
  }

  on_ready_ = nullptr;
  on_cancel_ = nullptr;
  on_event_.clear();
}

void Job::cancel() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (ready_) {
    return;
  }

  error_ = kCancelledError;
  error_message_ =  "Job cancelled";
  ready_ = true;
  lk.unlock();

  cv_.notify_all();

  if (on_cancel_) {
    on_cancel_();
  }

  if (on_ready_) {
    on_ready_();
  }

  on_ready_ = nullptr;
  on_cancel_ = nullptr;
  on_event_.clear();
}

void Job::wait() const {
  std::unique_lock<std::mutex> lk(mutex_);

  while (!ready_) {
    cv_.wait(lk);
  }
}

bool Job::waitFor(const Duration& timeout) const {
  std::unique_lock<std::mutex> lk(mutex_);

  if (ready_) {
    return true;
  }

  cv_.wait_for(lk, std::chrono::microseconds(timeout.microseconds()));
  return ready_;
}

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

