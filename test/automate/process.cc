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
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <string.h>
#include "process.h"

namespace eventql {
namespace test {

Process::Process() : pid_(-1), log_fd_(-1), running_(false) {
  stdin_pipe_[0] = -1;
  stdin_pipe_[1] = -1;
  stdout_pipe_[0] = -1;
  stdout_pipe_[1] = -1;
  stderr_pipe_[0] = -1;
  stderr_pipe_[1] = -1;
}

Process::~Process() {
  if (pid_ >= 0) {
    stop();
  }
}

void Process::runOrDie(
    const std::string filename,
    const std::vector<std::string>& argv,
    const std::vector<std::string>& envv /* = std::vector<std::string>{} */,
    const std::string& log_prefix /* = "" */,
    int log_fd /* = -1 */) {
  Process proc;
  if (log_fd >= 0) {
    proc.logDebug(log_prefix, log_fd);
  }

  auto rc = proc.start(filename, argv, envv);
  if (!rc.isSuccess()) {
    throw std::runtime_error(rc.getMessage());
  }

  proc.waitAndExpectSuccess();
}

ReturnCode Process::start(
    const std::string filename,
    const std::vector<std::string>& argv,
    const std::vector<std::string>& envv) {
  cmdline_.clear();
  for (const auto& env : envv) cmdline_ += env + " ";
  cmdline_ += filename;
  for (const auto& arg : argv) cmdline_ += " " + arg;

  if (log_fd_ >= 0) {
    dprintf(
        log_fd_,
        "[%s] Executing: %s\n",
        log_prefix_.c_str(),
        cmdline_.c_str());
  }

  if (pipe(stdin_pipe_) != 0 ||
      pipe(stdout_pipe_) != 0 ||
      pipe(stderr_pipe_) != 0 ||
      pipe(kill_pipe_) != 0) {
    return ReturnCode::error("EIO", "pipe() failed");
  }

  running_ = true;
  logtail_thread_ = std::thread(std::bind(&Process::runLogtail, this));
  pid_ = fork();

  if (pid_ < 0) {
    return ReturnCode::error("EIO", "fork() failed");
  }

  if (pid_ == 0) {
    std::vector<char*> argv_cstr;
    argv_cstr.emplace_back(const_cast<char*>(filename.c_str()));
    for (const auto& arg : argv) {
      argv_cstr.emplace_back(const_cast<char*>(arg.c_str()));
    }
    argv_cstr.emplace_back(nullptr);

    std::vector<char*> envv_cstr;
    for (const auto& env : envv) {
      envv_cstr.emplace_back(const_cast<char*>(env.c_str()));
    }
    envv_cstr.emplace_back(nullptr);

    if (dup2(stdin_pipe_[0], STDIN_FILENO) == -1) {
      dprintf(stderr_pipe_[1], "error: dup2() failed\n");
      exit(1);
    }

    if (dup2(stdin_pipe_[1], STDOUT_FILENO) == -1) {
      dprintf(stderr_pipe_[1], "error: dup2() failed\n");
      exit(1);
    }

    if (dup2(stderr_pipe_[1], STDERR_FILENO) == -1) {
      dprintf(stderr_pipe_[1], "error: dup2() failed\n");
      exit(1);
    }

    if (execve(filename.c_str(), argv_cstr.data(), envv_cstr.data()) == 0) {
      exit(0);
    } else {
      dprintf(stderr_pipe_[1], "error: dup2() failed\n");
      exit(1);
    }
  }

  return ReturnCode::success();
}

int Process::wait() {
  int status;
  waitpid(pid_, &status, 0);

  std::unique_lock<std::mutex> lk(running_mutex_);
  int rc = write(kill_pipe_[1], "X", 1);
  (void) rc;
  pid_ = -1;
  running_ = false;
  lk.unlock();

  if (logtail_thread_.joinable()) {
    logtail_thread_.join();
  }

  if (stdin_pipe_[0] >= 0) close(stdin_pipe_[0]);
  if (stdin_pipe_[1] >= 0) close(stdin_pipe_[1]);
  if (stdout_pipe_[0] >= 0) close(stdout_pipe_[0]);
  if (stdout_pipe_[1] >= 0) close(stdout_pipe_[1]);
  if (stderr_pipe_[0] >= 0) close(stderr_pipe_[0]);
  if (stderr_pipe_[1] >= 0) close(stderr_pipe_[1]);
  if (kill_pipe_[0] >= 0) close(kill_pipe_[0]);
  if (kill_pipe_[1] >= 0) close(kill_pipe_[1]);

  return status;
}

void Process::waitAndExpectSuccess() {
  auto status = wait();
  if (status != 0) {
    throw std::runtime_error("process exited with error: " + cmdline_);
  }
}

void Process::stop(int sig /* = SIGKILL */) {
  kill(pid_, sig);
  wait();
}

void Process::logDebug(std::string prefix, int fd) {
  log_prefix_ = prefix;
  log_fd_ = fd;
}

void Process::runLogtail() {
  while (true) {
    {
      std::unique_lock<std::mutex> lk(running_mutex_);
      if (!running_.load()) {
        return;
      }
    }

    struct pollfd p[3];
    p[0].fd = stdout_pipe_[0];
    p[0].events = POLLIN;
    p[1].fd = stderr_pipe_[0];
    p[1].events = POLLIN;
    p[2].fd = kill_pipe_[0];
    p[2].events = POLLIN;

    int poll_rc = poll(p, 3, 1000000);
    switch (poll_rc) {
      case -1:
        if (errno == EAGAIN || errno == EINTR) {
          continue;
        } else {
          if (running_.load() && log_fd_ >= 0) {
            dprintf(
                log_fd_,
                "[%s] logtail poll() failed: %s\n",
                log_prefix_.c_str(),
                strerror(errno));
          }
          return;
        }
      case 0:
        continue;
      default:
        break;
    }

    char buf[512];
    if (p[0].revents & POLLIN) {
      int rc = read(stdout_pipe_[0], buf, sizeof(buf));
      if (rc < 0) {
        if (log_fd_ >= 0) {
          dprintf(
              log_fd_,
              "[%s] logtail read() failed: %s\n",
              log_prefix_.c_str(),
              strerror(errno));
        }
        return;
      }

      stdout_buf_.append(buf, rc);
      for (;;) {
        auto pos = stdout_buf_.find("\n");
        if (pos == std::string::npos) {
          break;
        }

        auto line = stdout_buf_.substr(0, pos);
        stderr_buf_ = stdout_buf_.substr(pos + 1);
        if (log_fd_ >= 0) {
          dprintf(
              log_fd_,
              "[%s] [STDOUT] %s\n",
              log_prefix_.c_str(),
              line.c_str());
        }
      }

      continue;
    }

    if (p[1].revents & POLLIN) {
      int rc = read(stderr_pipe_[0], buf, sizeof(buf));
      if (rc < 0) {
        if (log_fd_ >= 0) {
          dprintf(
              log_fd_,
              "[%s] logtail read() failed: %s\n",
              log_prefix_.c_str(),
              strerror(errno));
        }
        return;
      }

      stderr_buf_.append(buf, rc);
      for (;;) {
        auto pos = stderr_buf_.find("\n");
        if (pos == std::string::npos) {
          break;
        }

        auto line = stderr_buf_.substr(0, pos);
        stderr_buf_ = stderr_buf_.substr(pos + 1);
        if (log_fd_ >= 0) {
          dprintf(
              log_fd_,
              "[%s] [STDERR] %s\n",
              log_prefix_.c_str(),
              line.c_str());
        }
      }

      continue;
    }

    if (p[2].revents & POLLIN) {
      return;
    }
  }
}

} // namespace test
} // namespace eventql

