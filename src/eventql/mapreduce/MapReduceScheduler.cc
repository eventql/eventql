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
#include "eventql/util/logging.h"
#include "eventql/util/http/HTTPFileDownload.h"
#include "eventql/mapreduce/MapReduceScheduler.h"
#include "eventql/transport/http/MapReduceService.h"

#include "eventql/eventql.h"

namespace eventql {

MapReduceScheduler::MapReduceScheduler(
    Session* session,
    RefPtr<MapReduceJobSpec> job,
    thread::ThreadPool* tpool,
    AnalyticsAuth* auth,
    const String& cachedir,
    size_t max_concurrent_tasks /* = kDefaultMaxConcurrentTasks */) :
    session_(session),
    job_(job),
    tpool_(tpool),
    auth_(auth),
    cachedir_(cachedir),
    max_concurrent_tasks_(max_concurrent_tasks) {}

void MapReduceScheduler::execute(const MapReduceShardList& shards) {
  if (shards.size() == 0) {
    return;
  }

  std::unique_lock<std::mutex> lk(mutex_);

  done_ = false;
  error_ = false;
  num_shards_running_ = 0;
  num_shards_completed_ = 0;
  shards_ = shards;
  shard_results_ = Vector<Option<MapReduceShardResult>>(shards_.size());
  shard_status_ = Vector<MapReduceShardStatus>(
      shards_.size(),
      MapReduceShardStatus::PENDING);

  shard_perms_.clear();
  for (size_t i = 0; i < shards_.size(); ++i) {
    shard_perms_.emplace_back(i);
  }

  std::random_shuffle(shard_perms_.begin(), shard_perms_.end());

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
  for (size_t j = 0; j < shards_.size(); ++j) {
    auto i = shard_perms_[j];

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
      Option<MapReduceShardResult> result;
      try {
        result = shard->task->execute(shard, base);
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

        shard_results_[i] = result;
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

void MapReduceScheduler::sendResult(const String& value) {
  job_->sendResult(value);
}

void MapReduceScheduler::sendLogline(const String& logline) {
  job_->sendLogline(logline);
}

Option<String> MapReduceScheduler::getResultURL(size_t task_index) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (task_index >= shards_.size()) {
    RAISEF(kIndexError, "invalid task index: $0", task_index);
  }

  if (shard_status_[task_index] != MapReduceShardStatus::COMPLETED) {
    RAISEF(kIndexError, "task is not completed: $0", task_index);
  }

  const auto& result = shard_results_[task_index];
  if (result.isEmpty()) {
    return None<String>();
  }

  auto result_path = FileUtil::joinPaths(
      cachedir_,
      StringUtil::format("mr-result-$0", result.get().result_id.toString()));

  return Some(StringUtil::format(
      "http://$0/api/v1/mapreduce/result/$1",
      result.get().host.addr.ipAndPort(),
      result.get().result_id.toString()));
}

Option<SHA1Hash> MapReduceScheduler::getResultID(size_t task_index) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (task_index >= shards_.size()) {
    RAISEF(kIndexError, "invalid task index: $0", task_index);
  }

  if (shard_status_[task_index] != MapReduceShardStatus::COMPLETED) {
    RAISEF(kIndexError, "task is not completed: $0", task_index);
  }

  const auto& result = shard_results_[task_index];
  if (result.isEmpty()) {
    return None<SHA1Hash>();
  }

  return Some(result.get().result_id);
}

Option<ReplicaRef> MapReduceScheduler::getResultHost(size_t task_index) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (task_index >= shards_.size()) {
    RAISEF(kIndexError, "invalid task index: $0", task_index);
  }

  if (shard_status_[task_index] != MapReduceShardStatus::COMPLETED) {
    RAISEF(kIndexError, "task is not completed: $0", task_index);
  }

  const auto& result = shard_results_[task_index];
  if (result.isEmpty()) {
    return None<ReplicaRef>();
  }

  return Some(result.get().host);
}

void MapReduceScheduler::downloadResult(
    size_t task_index,
    Function<void (const void*, size_t, const void*, size_t)> fn) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (task_index >= shards_.size()) {
    RAISEF(kIndexError, "invalid task index: $0", task_index);
  }

  if (shard_status_[task_index] != MapReduceShardStatus::COMPLETED) {
    RAISEF(kIndexError, "task is not completed: $0", task_index);
  }

  const auto& result = shard_results_[task_index];
  if (result.isEmpty()) {
    return;
  }

  auto url = StringUtil::format(
      "http://$0/api/v1/mapreduce/result/$1",
      result.get().host.addr.ipAndPort(),
      result.get().result_id.toString());

  auto api_token = auth_->encodeAuthToken(session_);

  http::HTTPMessage::HeaderList auth_headers;
  auth_headers.emplace_back(
      "Authorization",
      StringUtil::format("Token $0", api_token));

  auto req = http::HTTPRequest::mkGet(url, auth_headers);
  MapReduceService::downloadResult(req, fn);
}

RefPtr<MapReduceJobSpec> MapReduceScheduler::jobSpec() {
  return job_;
}

} // namespace eventql

