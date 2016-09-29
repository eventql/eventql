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
#pragma once
#include <thread>
#include "eventql/eventql.h"
#include "eventql/util/stdtypes.h"
#include "eventql/util/return_code.h"
#include "eventql/util/rolling_stat.h"
#include "eventql/transport/native/client_tcp.h"

namespace eventql {
namespace cli {

class BenchmarkStats {
public:

  BenchmarkStats();

  void addRequestStart();
  void addRequestComplete(bool is_success, uint64_t runtime_us);

  double getRollingRPS() const;
  double getRollingAverageRuntime() const;
  uint64_t getTotalRequestCount() const;
  uint64_t getRunningRequestCount() const;
  uint64_t getTotalErrorCount() const;
  double getTotalErrorRate() const;

protected:

  RollingStat rolling_rps_;
  RollingStat rolling_avg_runtime_;
  uint64_t total_request_count_;
  uint64_t running_request_count_;
  uint64_t total_error_count_;
  mutable std::mutex mutex_;
};

class Benchmark {
public:

  using RequestCallbackType =
      std::function<ReturnCode (
          native_transport::TCPClient* conn,
          uint64_t sequence)>;

  using ProgressCallbackType = std::function<void (BenchmarkStats* stats)>;

  static const uint64_t kDefaultProgressRateLimit = kMicrosPerSecond;

  Benchmark(
      size_t num_threads,
      bool ignore_errors,
      size_t rate,
      size_t remaining_requests = size_t(-1),
      bool disable_keepalive = false);

  void setRequestHandler(RequestCallbackType handler);
  void setProgressCallback(ProgressCallbackType cb);
  void setProgressRateLimit(uint64_t rate_limit_us);

  ReturnCode connect(
      const std::string& host,
      uint64_t port,
      const std::vector<std::pair<std::string, std::string>>& auth_data);

  ReturnCode run();
  void kill();

  const BenchmarkStats* getStats() const;

protected:

  void runThread(size_t idx);
  bool getRequestSlot(size_t idx);

  const size_t num_threads_;
  bool ignore_errors_;
  const uint64_t rate_;
  uint64_t rate_limit_interval_;
  size_t remaining_requests_;
  bool disable_keepalive_;
  ReturnCode status_;
  size_t threads_running_;
  uint64_t last_request_time_;
  std::atomic<uint64_t> sequence_;
  RequestCallbackType request_handler_;
  ProgressCallbackType on_progress_;
  uint64_t progress_rate_limit_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::vector<std::thread> threads_;
  std::vector<std::unique_ptr<native_transport::TCPClient>> clients_;
  BenchmarkStats stats_;
  std::string host_;
  uint64_t port_;
  std::vector<std::pair<std::string, std::string>> auth_data_;
};

ReturnCode benchmark_insert(
    native_transport::TCPClient* conn,
    uint64_t sequence,
    const std::string& database,
    const std::string& table,
    const std::string& payload,
    size_t batch_size);

ReturnCode benchmark_query(
    native_transport::TCPClient* conn,
    uint64_t sequence,
    const std::string& database,
    const std::string& payload);

} //cli
} //eventql

