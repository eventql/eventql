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
#pragma once
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <sys/types.h>
#include <signal.h>
#include "eventql/util/return_code.h"

namespace eventql {
namespace test {

class Process {
public:

  Process();
  Process(const Process& other) = delete;
  ~Process();

  Process& operator=(const Process& other) = delete;

  ReturnCode start(
      const std::string filename,
      const std::vector<std::string>& argv,
      const std::vector<std::string>& envv = std::vector<std::string> {});

  int wait();
  void waitAndExpectSuccess();

  void stop(int signal = SIGKILL);

  void storeSTDOUT(std::string* stdout_dst);
  void storeSTDERR(std::string* stderr_dst);
  void logDebug(std::string prefix, int fd);

protected:

  void runLogtail();

  int pid_;
  int stdin_pipe_[2];
  int stdout_pipe_[2];
  int stderr_pipe_[2];
  int kill_pipe_[2];
  std::string stdout_buf_;
  std::string stderr_buf_;
  std::string cmdline_;
  std::thread logtail_thread_;
  std::string log_prefix_;
  int log_fd_;
  std::mutex running_mutex_;
  std::atomic<bool> running_;
};

} // namespace test
} // namespace eventql

