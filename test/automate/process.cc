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
#include <unistd.h>
#include "process.h"

namespace eventql {
namespace test {

Process::Process() : pid_(-1) {
  stdin_pipe_[0] = -1;
  stdin_pipe_[1] = -1;
  stdout_pipe_[0] = -1;
  stdout_pipe_[1] = -1;
  stderr_pipe_[0] = -1;
  stderr_pipe_[1] = -1;
}

Process::~Process() {
  if (stdin_pipe_[0] >= 0) close(stdin_pipe_[0]);
  if (stdin_pipe_[1] >= 0) close(stdin_pipe_[1]);
  if (stdout_pipe_[0] >= 0) close(stdout_pipe_[0]);
  if (stdout_pipe_[1] >= 0) close(stdout_pipe_[1]);
  if (stderr_pipe_[0] >= 0) close(stderr_pipe_[0]);
  if (stderr_pipe_[1] >= 0) close(stderr_pipe_[1]);
}

ReturnCode Process::start(
    const std::string filename,
    const std::vector<std::string>& argv,
    const std::vector<std::string>& envv) {
  if (pipe(stdin_pipe_) != 0 ||
      pipe(stdout_pipe_) != 0 ||
      pipe(stderr_pipe_) != 0) {
    return ReturnCode::error("EIO", "pipe() failed");
  }

  pid_ = fork();

  if (pid_ < 0) {
    return ReturnCode::error("EIO", "fork() failed");
  }

  if (pid_ == 0) {
    std::vector<char*> argv_cstr;
    for (const auto& arg : argv) {
      argv_cstr.emplace_back(const_cast<char*>(arg.c_str()));
    }

    std::vector<char*> envv_cstr;
    for (const auto& env : envv) {
      envv_cstr.emplace_back(const_cast<char*>(env.c_str()));
    }

    if (dup2(stdin_pipe_[0], STDIN_FILENO) != 0) {
      dprintf(stderr_pipe_[0], "error: dup2() failed\n");
      exit(1);
    }

    if (dup2(stdin_pipe_[1], STDOUT_FILENO) != 0) {
      dprintf(stderr_pipe_[0], "error: dup2() failed\n");
      exit(1);
    }

    if (dup2(stderr_pipe_[1], STDERR_FILENO) != 0) {
      dprintf(stderr_pipe_[0], "error: dup2() failed\n");
      exit(1);
    }

    if (execve(filename.c_str(), argv_cstr.data(), envv_cstr.data()) == 0) {
      exit(0);
    } else {
      dprintf(stderr_pipe_[0], "error: dup2() failed\n");
      exit(1);
    }
  }

  return ReturnCode::success();
}

void Process::stop() {

}

} // namespace test
} // namespace eventql

