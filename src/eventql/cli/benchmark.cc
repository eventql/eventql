/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include "eventql/cli/benchmark.h"
#include "eventql/util/wallclock.h"

/**
 * TODO:
 *   - Benchmark.setRequestHandler(Fun<void (BenchmarkStats*>)
 *   - pass arguments
 *   - benchmark stats: Benchmark::getStats()
 *   - print benchmark stats
 *   - Benchmark::connect()
 *   - benchmark stats: moving average
 *   -----
 *   - Benchmark::runRequest()
 *   - improve thread load balancing/scheudling
 */

namespace eventql {
namespace cli {

BenchmarkStats::BenchmarkStats() : total_requests_(0) {
  buckets_.resize(kNumBuckets);
  buckets_begin_ = 0;
}

void BenchmarkStats::addRequest(
    bool is_success,
    size_t t_id,
    uint64_t start_time) {
  ++total_requests_;

  auto twindow = start_time / kTimeWindowSize * kTimeWindowSize;
  //start first time window
  if (buckets_[buckets_begin_].time == 0) {
    buckets_[buckets_begin_].time = twindow;
    buckets_[buckets_begin_].num_requests = 0;

  //start new time window
  } else if (buckets_[buckets_begin_].time != twindow) {
    if (buckets_begin_ == kNumBuckets - 1) {
      buckets_begin_ = 0;
    } else {
      ++buckets_begin_;
    }
    buckets_[buckets_begin_].time = twindow;
    buckets_[buckets_begin_].num_requests = 0;
  }

  buckets_[buckets_begin_].num_requests += 1;
}

std::string BenchmarkStats::toString() const {
  //calculate moving average for the last minute
  auto start = MonotonicClock::now() - kMicrosPerMinute;
  uint64_t num_requests = 0;
  size_t bucket_ctr = 0;
  for (size_t i = 0; i < buckets_.size(); ++i) {
    if (buckets_[i].time < start) {
      continue;
    }

    ++bucket_ctr;
    num_requests += buckets_[i].num_requests;
  }

  return StringUtil::format(
      "total $0 --- avg rps $1",
      total_requests_,
      (double) num_requests / (bucket_ctr * 10));
}

// FIXME pass proper arguments
Benchmark::Benchmark() :
    num_threads_(4),
    status_(ReturnCode::success()),
    threads_running_(0),
    last_request_time_(0),
    rate_limit_interval_(1000000),
    remaining_requests_(20) {
  threads_.resize(num_threads_);
}

void Benchmark::setProgressCallback(std::function<void ()> cb) {
  on_progress_ = cb;
}

ReturnCode Benchmark::run() {
  for (size_t i = 0; i < num_threads_; ++i) {
    threads_[i] = std::thread(std::bind(&Benchmark::runThread, this, i));
    ++threads_running_;
  }

  {
    std::unique_lock<std::mutex> lk(mutex_);
    while (threads_running_ > 0) {
      cv_.wait_for(lk, std::chrono::microseconds(kMicrosPerSecond / 10)); // FIXME
      if (on_progress_) {
        on_progress_(); //FIXME pass BenchmarkStats
      }
      // FIXME rate limit progress callback?
    }
  }

  for (auto& t : threads_) {
    if (t.joinable()) {
      t.join();
    }
  }

  return status_;
}

void Benchmark::kill() {
  std::unique_lock<std::mutex> lk(mutex_);
  status_ = ReturnCode::error("ERUNTIME", "Benchmark aborted...");
  cv_.notify_all();
}

BenchmarkStats* Benchmark::getStats() {
  return &stats_; //FIXME
}

void Benchmark::runThread(size_t idx) {
  while (getRequestSlot(idx)) {
    // FIXME record start time
    auto rc = ReturnCode::success(); // FIXME call request handler/cb
    // FIXME record end time

    std::unique_lock<std::mutex> lk(mutex_);
    stats_.addRequest(rc.isSuccess(), idx, MonotonicClock::now());

    if (!rc.isSuccess()) {
      status_ = rc;
      cv_.notify_all();
      break;
    }
  }

  std::unique_lock<std::mutex> lk(mutex_);
  --threads_running_;
  cv_.notify_all();
}

// FIXME: this will "randomly" (not really) select one of our threads to run
// the next request. we should improve this to do some kind of best-effort
// round robin/load balancing
bool Benchmark::getRequestSlot(size_t idx) {
  std::unique_lock<std::mutex> lk(mutex_);
  for (;;) {
    if (!status_.isSuccess()) {
      return false;
    }

    if (remaining_requests_ == 0) {
      return false;
    }

    if (rate_limit_interval_ == 0) {
      return true;
    }

    auto now = MonotonicClock::now();
    if (last_request_time_ + rate_limit_interval_ <= now) {
      last_request_time_ = now;
      if (remaining_requests_ != size_t(-1)) {
        --remaining_requests_;
      }
      break;
    }

    auto wait = (last_request_time_ + rate_limit_interval_) - now;
    cv_.wait_for(lk, std::chrono::microseconds(wait));
  }

  return true;
}

} //cli
} //eventql

