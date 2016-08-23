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
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ctime>
#include <sys/time.h>
#include <sys/types.h>
#include <thread>
#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif
#include "eventql/util/http/httpserverconnection.h"
#include "eventql/eventql.h"
#include "eventql/server/listener.h"
#include "eventql/util/return_code.h"
#include "eventql/util/logging.h"

namespace eventql {

namespace {

class LocalTaskScheduler : public TaskScheduler {
public:

  void run(std::function<void()> task) override {
    task();
  }

  void runOnReadable(std::function<void()> task, int fd) override {
    fd_set op_read, op_write;
    FD_ZERO(&op_read);
    FD_ZERO(&op_write);
    FD_SET(fd, &op_read);

    auto res = select(fd + 1, &op_read, &op_write, &op_read, NULL);

    if (res == 0) {
      RAISE(kIOError, "unexpected timeout while select()ing");
    }

    if (res == -1) {
      RAISE_ERRNO(kIOError, "select() failed");
    }

    task();
  }

  void runOnWritable(std::function<void()> task, int fd) override{
    run([task, fd] () {
      fd_set op_read, op_write;
      FD_ZERO(&op_read);
      FD_ZERO(&op_write);
      FD_SET(fd, &op_write);

      auto res = select(fd + 1, &op_read, &op_write, &op_write, NULL);

      if (res == 0) {
        RAISE(kIOError, "unexpected timeout while select()ing");
      }

      if (res == -1) {
        RAISE_ERRNO(kIOError, "select() failed");
      }

      task();
    });
  }

  void runOnWakeup(
      std::function<void()> task,
      Wakeup* wakeup,
      long generation) override {
    run([task, wakeup, generation] () {
      wakeup->waitForWakeup(generation);
      task();
    });
  }

};

static LocalTaskScheduler local_scheduler;

} // namespace

uint64_t getMonoTime() {
#ifdef __MACH__
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  return std::uint64_t(mts.tv_sec) * 1000000 + mts.tv_nsec / 1000;
#else
  timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    logFatal("clock_gettime(CLOCK_MONOTONIC) failed");
    abort();
  } else {
    return std::uint64_t(ts.tv_sec) * 1000000 + ts.tv_nsec / 1000;
  }
#endif
}

Listener::Listener(
    http::HTTPHandlerFactory* http_handler) :
    http_handler_(http_handler),
    connect_timeout_(2 * kMicrosPerSecond), 
    running_(true),
    ssock_(-1) {}

ReturnCode Listener::bind(int listen_port) {
  ssock_ = ::socket(AF_INET, SOCK_STREAM, 0);
  if (ssock_ == 0) {
    return ReturnCode::error(
        "IOERR",
        "create socket() failed: %s",
        strerror(errno));
  }

  int opt = 1;
  if (::setsockopt(ssock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    return ReturnCode::error(
        "IOERR",
        "setsockopt(SO_REUSEADDR) failed: %s",
        strerror(errno));
  }

  int flags = ::fcntl(ssock_, F_GETFL, 0);
  flags = flags | O_NONBLOCK;

  if (::fcntl(ssock_, F_SETFL, flags) != 0) {
    return ReturnCode::error(
        "IOERR",
        "fcntl(F_SETFL, O_NONBLOCK) failed: %s",
        strerror(errno));
  }

  struct sockaddr_in addr;
  memset((char *) &addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(listen_port);
  if (::bind(ssock_, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    return ReturnCode::error(
        "IOERR",
        "bind() failed: %s",
        strerror(errno));
  }

  if (::listen(ssock_, 1024) == -1) {
    return ReturnCode::error(
        "IOERR",
        "listen() failed: %s",
        strerror(errno));
  }

  logNotice("eventql", "Listening on port $0", listen_port);
  return ReturnCode::success();
}

void Listener::run() {
  while (running_.load()) {
    fd_set op_read, op_write, op_error;
    FD_ZERO(&op_read);
    FD_ZERO(&op_write);
    FD_ZERO(&op_error);

    int max_fd = ssock_;
    FD_SET(ssock_, &op_read);

    auto now = getMonoTime();
    while (!connections_.empty()) {
      if (connections_.front().accepted_at + connect_timeout_ > now) {
        break;
      }

      logError(
          "eventql",
          "timeout while reading first byte, closing connection");

      close(connections_.front().fd);
      connections_.pop_front();
    }

    uint64_t timeout = 0;
    if (!connections_.empty()) {
      timeout = (connections_.front().accepted_at + connect_timeout_) - now;
    }

    for (const auto& c : connections_) {
      if (c.fd > max_fd) {
        max_fd = c.fd;
      }

      FD_SET(c.fd, &op_read);
    }

    int res;
    if (timeout) {
      struct timeval tv;
      tv.tv_sec = timeout / kMicrosPerSecond;
      tv.tv_usec = timeout % kMicrosPerSecond;
      res = select(max_fd + 1, &op_read, &op_write, &op_error, &tv);
    } else {
      res = select(max_fd + 1, &op_read, &op_write, &op_error, NULL);
    }

    switch (res) {
      case 0:
        continue;
      case -1:
        if (errno == EINTR) {
          continue;
        }

        logError("eventql", "select() failed");
        return;
    }

    if (FD_ISSET(ssock_, &op_read)) {
      int conn_fd = ::accept(ssock_, NULL, NULL);
      if (conn_fd < 0) {
        logError("eventql", "accept() failed");
      }

      int flags = ::fcntl(conn_fd, F_GETFL, 0);
      flags = flags | O_NONBLOCK;

      if (::fcntl(conn_fd, F_SETFL, flags) != 0) {
        logError(
            "eventql",
            "fcntl(F_SETFL, O_NONBLOCK) failed, closing connection: $0",
            strerror(errno));

        close(conn_fd);
        return;
      }

      EstablishingConnection c;
      c.fd = conn_fd;
      c.accepted_at = getMonoTime();
      connections_.emplace_back(c);
    }

    auto iter = connections_.begin();
    while (iter != connections_.end()) {
      if (FD_ISSET(iter->fd, &op_read)) {
        open(iter->fd);
        iter = connections_.erase(iter);
      } else {
        iter++;
      }
    }
  }
}

void Listener::shutdown() {
}

void Listener::open(int fd) {
  char first_byte;
  auto res = ::read(fd, &first_byte, 1);
  if (res != 1) {
    logError(
        "eventql",
        "error while reading first byte, closing connection: $0",
        strerror(errno));

    close(fd);
    return;
  }

  int flags = ::fcntl(fd, F_GETFL, 0);
  flags = flags & ~O_NONBLOCK;

  if (::fcntl(fd, F_SETFL, flags) != 0) {
    logError(
        "eventql",
        "fcntl(F_SETFL, O_NONBLOCK) failed, closing connection: $0",
        strerror(errno));

    close(fd);
    return;
  }

  auto t = std::thread([fd, first_byte, this] {
    logDebug("eventql", "Opening new http connection; fd=$0", fd);
    auto http_conn = mkRef(
        new http::HTTPServerConnection(
            http_handler_,
            ScopedPtr<net::TCPConnection>(new net::TCPConnection(fd)),
            &local_scheduler,
            &http_stats_));

    try {
      http_conn->start(std::string(&first_byte, 1));
    } catch (const std::exception& e){
      logError(
          "eventql",
          "HTTP connection error: $0",
          e.what());
    }
  });

  t.detach();
}

} // namespace eventql
