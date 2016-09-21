/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/logging.h>
#include <eventql/util/application.h>
#include "eventql/eventql.h"
#include "eventql/db/leader.h"

namespace eventql {

Leader::Leader(
    ConfigDirectory* cdir,
    uint64_t rebalance_interval) :
    cdir_(cdir),
    rebalance_(cdir),
    rebalance_interval_(rebalance_interval) {}

Leader::~Leader() {
  assert(thread_running_ == false);
}

bool Leader::runLeaderProcedure() {
  if (!cdir_->electLeader()) {
    return false;
  }

  logDebug("evqld", "Local node is running the leader procedure");

  auto rc = rebalance_.runOnce();
  if (!rc.isSuccess()) {
    logError("evqld", "Rebalance error: $0", rc.message());
  }

  return true;
}

void Leader::startLeaderThread() {
  thread_running_ = true;

  thread_ = std::thread([this] {
    Application::setCurrentThreadName("evqld-gc");

    std::unique_lock<std::mutex> lk(mutex_);
    while (thread_running_) {
      lk.unlock();

      try {
        runLeaderProcedure();
      } catch (const std::exception& e) {
        logError("evqld", e, "Error in Leader thread");
      }

      lk.lock();
      cv_.wait_for(
          lk,
          std::chrono::microseconds(rebalance_interval_),
          [this] () { return !thread_running_; });
    }
  });
}

void Leader::stopLeaderThread() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (!thread_running_) {
    return;
  }

  thread_running_ = false;
  cv_.notify_all();
}

} // namespace eventql

