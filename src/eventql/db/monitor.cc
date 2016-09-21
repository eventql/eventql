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
#include <sys/statvfs.h>
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/logging.h>
#include <eventql/util/application.h>
#include "eventql/eventql.h"
#include "eventql/db/monitor.h"
#include <eventql/db/database.h>
#include <eventql/util/wallclock.h>
#include <eventql/server/server_stats.h>

namespace eventql {

Monitor::Monitor(DatabaseContext* dbctx) : dbctx_(dbctx) {}

Monitor::~Monitor() {
  assert(thread_running_ == false);
}

ReturnCode Monitor::runMonitorProcedure() {
  auto zs = evqld_stats();

  /* get disk used and capacity */
  uint64_t disk_used = FileUtil::du_c(dbctx_->db_path);
  uint64_t disk_available = 0;
  {
    uint64_t disk_capacity = 0;
    auto disk_capacity_cfg = dbctx_->config->getInt("server.disk_capacity");
    if (!disk_capacity_cfg.isEmpty()) {
      disk_capacity = disk_capacity_cfg.get();
    }

    struct statvfs svfs;
    memset(&svfs, 0, sizeof(svfs));
    if (statvfs(dbctx_->db_path.c_str(), &svfs) == 0) {
      auto disk_capacity_real = disk_used + (svfs.f_bavail * svfs.f_frsize);
      if (disk_capacity == 0 ||
          (disk_capacity_real > 0 && disk_capacity_real < disk_capacity)) {
        disk_capacity = disk_capacity_real;
      }
    }

    if (disk_capacity > disk_used) {
      disk_available = disk_capacity - disk_used;
    }
  }

  /* calculate load factor */
  uint64_t partitions_assigned = zs->num_partitions.get();
  uint64_t partitions_loaded = zs->num_partitions_opened.get();
  uint64_t partitions_loading = zs->num_partitions_loading.get();
  if (partitions_loaded > partitions_loading) {
    partitions_loaded -= partitions_loading;
  }

  /* publish server stats */
  logDebug(
      "evqld",
      "LOAD INFO: disk_used=$0MB disk_avail=$1MB partitions=$2/$3",
      disk_used / 0x100000,
      disk_available / 0x100000,
      partitions_loaded,
      partitions_assigned);

  ServerStats sstats;
  sstats.set_disk_used(disk_used);
  sstats.set_disk_available(disk_available);
  sstats.set_partitions_loaded(partitions_loaded);
  sstats.set_partitions_assigned(partitions_assigned);
  sstats.set_buildinfo(StringUtil::format("$0 ($1)", kVersionString, kBuildID));
  return dbctx_->config_directory->publishServerStats(sstats);
}

void Monitor::startMonitorThread() {
  thread_running_ = true;

  thread_ = std::thread([this] {
    Application::setCurrentThreadName("evqld-monitor");

    std::unique_lock<std::mutex> lk(mutex_);
    while (thread_running_) {
      lk.unlock();

      auto rc = ReturnCode::success();
      auto t0 = MonotonicClock::now();
      try {
        rc = runMonitorProcedure();
      } catch (const std::exception& e) {
        rc = ReturnCode::exception(e);
      }
      auto t1 = MonotonicClock::now();

      if (!rc.isSuccess()) {
        logError("evqld", "Error in Monitor thread: $0", rc.getMessage());
      }

      lk.lock();

      auto runtime = t1 - t0;
      auto delay = dbctx_->config->getInt(
          "server.loadinfo_publish_interval").get();

      if (runtime > delay) {
        delay = 0;
      } else {
        delay -= runtime;
      }

      cv_.wait_for(
          lk,
          std::chrono::microseconds(delay),
          [this] () { return !thread_running_; });
    }
  });
}

void Monitor::stopMonitorThread() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (!thread_running_) {
    return;
  }

  thread_running_ = false;
  cv_.notify_all();
}

} // namespace eventql

