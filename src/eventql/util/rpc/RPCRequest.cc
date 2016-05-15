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
#include "eventql/util/stdtypes.h"
#include "eventql/util/rpc/RPCRequest.h"
#include "eventql/util/rpc/RPCContext.h"

namespace rpc {

RPCRequest::RPCRequest(
    Function<void (RPCContext* ctx)> call_fn) :
    call_fn_(call_fn),
    ready_(false),
    error_(nullptr),
    running_(false) {}

void RPCRequest::run() {
  {
    std::unique_lock<std::mutex> lk(mutex_);
    if (ready_ || running_) {
      RAISE(kRuntimeError, "refusing to run a finished/running job");
    }

    running_ = true;
  }

  RPCContext ctx(this);
  try {
    call_fn_(&ctx);
  } catch (const StandardException& e) {
    returnError(e);
    return;
  }

  return returnSuccess();
}

void RPCRequest::onReady(Function<void ()> fn) {
  std::unique_lock<std::mutex> lk(mutex_);

  if (ready_) {
    fn();
  } else {
    on_ready_ = fn;
  }
}

void RPCRequest::returnSuccess() {
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

void RPCRequest::returnError(const StandardException& e) {
  try {
    auto rte = dynamic_cast<const Exception&>(e);
    returnError(rte.getType(), rte.getMessage());
  } catch (const std::exception& cast_error) {
    returnError(kRuntimeError, e.what());
  }
}

void RPCRequest::returnError(ExceptionType error_type, const String& message) {
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

void RPCRequest::cancel() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (ready_) {
    return;
  }

  error_ = kCancelledError;
  error_message_ =  "RPCRequest cancelled";
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

void RPCRequest::wait() const {
  std::unique_lock<std::mutex> lk(mutex_);

  while (!ready_) {
    cv_.wait(lk);
  }
}

bool RPCRequest::waitFor(const Duration& timeout) const {
  std::unique_lock<std::mutex> lk(mutex_);

  if (ready_) {
    return true;
  }

  cv_.wait_for(lk, std::chrono::microseconds(timeout.microseconds()));
  return ready_;
}

} // namespace rpc

