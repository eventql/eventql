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
#ifndef libstx_EV_EVENTLOOP_H
#define libstx_EV_EVENTLOOP_H
#include <list>
#include <sys/select.h>
#include <thread>
#include <vector>
#include "eventql/util/thread/taskscheduler.h"

namespace thread {

class EventLoop : public TaskScheduler {
public:
  void run(std::function<void()> task) override;
  //void runAsync(std::function<void()> task) override;

  void runOnReadable(std::function<void()> task, int fd) override;
  void runOnReadable(
      std::function<void()> task,
      int fd,
      uint64_t timeout_micros,
      std::function<void()> on_timeout);

  void runOnWritable(std::function<void()> task, int fd) override;
  void runOnWakeup(
      std::function<void()> task,
      Wakeup* wakeup,
      long wakeup_generation) override;

  void cancelFD(int fd) override;

  EventLoop();
  ~EventLoop();
  void run();
  void runOnce();
  void shutdown();
  void wakeup();

protected:

  void poll();
  void setupRunQWakeupPipe();
  void onRunQWakeup();
  void appendToRunQ(std::function<void()> task);

  fd_set op_read_;
  fd_set op_write_;
  fd_set op_error_;
  int max_fd_;
  std::atomic<bool> running_;
  int runq_wakeup_pipe_[2];
  std::list<std::function<void()>> runq_;
  std::mutex runq_mutex_;
  std::thread::id threadid_;
  std::vector<std::function<void()>> callbacks_;
  size_t num_fds_;
};

}
#endif
