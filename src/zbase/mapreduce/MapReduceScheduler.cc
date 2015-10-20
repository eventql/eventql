/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/logging.h"
#include "zbase/mapreduce/MapReduceScheduler.h"

using namespace stx;

namespace zbase {

MapReduceScheduler::MapReduceScheduler(
    RefPtr<MapReduceJobSpec> job,
    const MapReduceShardList& shards,
    thread::ThreadPool* tpool,
    size_t max_concurrent_tasks /* = kDefaultMaxConcurrentTasks */) :
    job_(job),
    shards_(shards),
    shard_status_(shards_.size(), MapReduceShardStatus::PENDING),
    shard_results_(shards_.size()),
    tpool_(tpool),
    max_concurrent_tasks_(max_concurrent_tasks),
    done_(false),
    error_(false),
    num_shards_running_(0),
    num_shards_completed_(0) {}

void MapReduceScheduler::execute() {
  std::unique_lock<std::mutex> lk(mutex_);

  for (;;) {
    logDebug(
        "z1.mapreduce",
        "Running job; progress=$0/$1 ($2 runnning)",
        num_shards_completed_,
        shards_.size(),
        num_shards_running_);

    job_->updateProgress(MapReduceJobStatus{
      .num_tasks_total = shards_.size(),
      .num_tasks_completed = num_shards_completed_,
      .num_tasks_running = num_shards_running_
    });

    if (error_) {
      auto err_str = StringUtil::join(errors_, ", ");
      RAISEF(
          kRuntimeError,
          "MapReduce execution failed: $0",
          err_str);
    }


    if (done_) {
      break;
    }

    if (startJobs() > 0) {
      continue;
    }

    cv_.wait(lk);
  }
}

size_t MapReduceScheduler::startJobs() {
  if (num_shards_running_ >= max_concurrent_tasks_) {
    return 0;
  }

  if (num_shards_completed_ + num_shards_running_ >= shards_.size()) {
    return 0;
  }

  size_t num_started = 0;
  for (size_t i = 0; i < shards_.size(); ++i) {
    if (shard_status_[i] != MapReduceShardStatus::PENDING) {
      continue;
    }

    bool ready = true;
    for (auto dep : shards_[i]->dependencies) {
      if (shard_status_[dep] != MapReduceShardStatus::COMPLETED) {
        ready = false;
      }
    }

    if (!ready) {
      continue;
    }

    ++num_shards_running_;
    ++num_started;
    shard_status_[i] = MapReduceShardStatus::RUNNING;
    auto shard = shards_[i];

    auto base = mkRef(this);
    tpool_->run([this, i, shard, base] {
      bool error = false;
      String error_str;
      try {
        shard->task->execute(shard, base);
      } catch (const StandardException& e) {
        error_str = e.what();
        error = true;

        logError(
            "z1.mapreduce",
            e,
            "MapReduceTaskShard failed");
      }

      {
        std::unique_lock<std::mutex> lk(mutex_);
        shard_status_[i] = error
            ? MapReduceShardStatus::ERROR
            : MapReduceShardStatus::COMPLETED;

        --num_shards_running_;
        if (++num_shards_completed_ == shards_.size()) {
          done_ = true;
        }

        if (error) {
          error_ = true;
          errors_.emplace_back(error_str);
        }

        lk.unlock();
        cv_.notify_all();
      }
    });

    if (num_shards_running_ >= max_concurrent_tasks_) {
      break;
    }
  }

  return num_started;
}

void MapReduceScheduler::sendResult(const String& key, const String& value) {
  job_->sendResult(key, value);
}


} // namespace zbase

