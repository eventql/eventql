/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <assert.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include "eventql/util/logging.h"
#include "eventql/util/wallclock.h"
#include "eventql/sql/scheduler/aggregation_scheduler.h"
#include "eventql/transport/native/frames/hello.h"

namespace eventql {

AggregationScheduler::AggregationScheduler(
    ConfigDirectory* config,
    size_t max_concurrent_tasks,
    size_t max_concurrent_tasks_per_host) :
    config_(config),
    max_concurrent_tasks_(max_concurrent_tasks),
    max_concurrent_tasks_per_host_(max_concurrent_tasks_per_host),
    num_parts_(0),
    num_parts_complete_(0),
    num_parts_running_(0),
    io_timeout_(kMicrosPerSecond),
    idle_timeout_(kMicrosPerSecond) {}

//void Aggregatio::addLocalPart(
//    const csql::GroupByNode* query);
//
void AggregationScheduler::addRemotePart(
    const csql::GroupByNode* query,
    const std::vector<std::string>& hosts) {
  assert(hosts.size() > 0);
  auto part = new AggregationPart();
  part->state = AggregationPartState::INIT;
  part->hosts = hosts;
  runq_.push_back(part);
  ++num_parts_;
}

void AggregationScheduler::setResultCallback(
    const std::function<void ()> fn) {

}

ReturnCode AggregationScheduler::handleFrame(
    Connection* connection,
    uint16_t opcode,
    uint16_t flags,
    const char* payload,
    size_t payload_size) {
  iputs("got fraem: $0 ($1)", opcode, payload_size);
  return ReturnCode::success();
}

ReturnCode AggregationScheduler::handleHandshake(Connection* connection) {
  native_transport::HelloFrame f_hello;
  f_hello.writeToString(&connection->write_buf);
  return ReturnCode::success();
}

ReturnCode AggregationScheduler::execute() {
  fd_set op_read, op_write, op_error;
  while (num_parts_complete_ < num_parts_) {
    for (size_t i = num_parts_running_; i < max_concurrent_tasks_; ++i) {
      auto rc = startNextPart();
      if (!rc.isSuccess()) {
        return rc;
      }
    }

    int max_fd = 0;
    FD_ZERO(&op_read);
    FD_ZERO(&op_error);
    FD_ZERO(&op_write);

    auto now = MonotonicClock::now();
    auto timeout = now + idle_timeout_;
    for (const auto& connection : connections_) {
      FD_SET(connection.fd, &op_read);
      max_fd = std::max(max_fd, connection.fd);

      if (connection.read_timeout > 0 && connection.read_timeout < timeout) {
        timeout = connection.read_timeout;
      }

      if (connection.write_buf.size() > 0 ||
          connection.state == ConnectionState::CONNECTING) {
        FD_SET(connection.fd, &op_write);
        max_fd = std::max(max_fd, connection.fd);
      }

      if (connection.write_timeout > 0 && connection.write_timeout < timeout) {
        timeout = connection.write_timeout;
      }
    }

    auto timeout_diff = timeout > now ? timeout - now : 0;
    struct timeval tv;
    tv.tv_sec = timeout_diff / kMicrosPerSecond;
    tv.tv_usec = timeout_diff % kMicrosPerSecond;

    int res = select(max_fd + 1, &op_read, &op_write, &op_error, &tv);
    if (res == -1 && errno != EINTR) {
      return ReturnCode::error(
          "EIO",
          "select() failed: %s",
          strerror(errno));
    }

    now = MonotonicClock::now();
    for (auto conn = connections_.begin(); conn != connections_.end(); ) {
      if ((conn->read_timeout > 0 && conn->read_timeout <= now) ||
          (conn->write_timeout > 0 && conn->write_timeout <= now)) {
        logDebug("evqld", "Connection timed out");

        auto part = conn->part;
        closeConnection(&*conn);
        conn = connections_.erase(conn);
        auto rc = failPart(part);
        if (rc.isSuccess()) {
          continue;
        } else {
          return rc;
        }
      }

      if (FD_ISSET(conn->fd, &op_read)) {
        conn->read_timeout = 0;

        auto rc = performRead(&*conn);
        if (!rc.isSuccess()) {
          auto part = conn->part;
          closeConnection(&*conn);
          conn = connections_.erase(conn);
          rc = failPart(part);
          if (rc.isSuccess()) {
            continue;
          } else {
            return rc;
          }
        }

        if (conn->state == ConnectionState::CLOSE) {
          closeConnection(&*conn);
          conn = connections_.erase(conn);
          continue;
        }
      }

      if (FD_ISSET(conn->fd, &op_write)) {
        conn->write_timeout = 0;

        auto rc = performWrite(&*conn);
        if (!rc.isSuccess()) {
          auto part = conn->part;
          closeConnection(&*conn);
          conn = connections_.erase(conn);
          rc = failPart(part);
          if (rc.isSuccess()) {
            continue;
          } else {
            return rc;
          }
        }
      }

      ++conn;
    }
  }

  return ReturnCode::success();
}

ReturnCode AggregationScheduler::performRead(Connection* connection) {
  size_t batch_size = 4096;
  auto begin = connection->read_buf.size();
  connection->read_buf.resize(begin + batch_size);

  int ret = ::read(
      connection->fd,
      (void*) (&connection->read_buf[0] + begin),
      batch_size);

  if (ret == 0) {
    return ReturnCode::error("EIO", "unexpected end of file");
  }

  if (ret == -1) {
    if (ret == EAGAIN) {
      connection->read_buf.resize(begin);
      return ReturnCode::success();
    }

    return ReturnCode::error(
        "EIO",
        "write() failed: %s",
        strerror(errno));
  }

  connection->read_buf.resize(begin + ret);
  if (connection->read_buf.size() < 8) {
    return ReturnCode::success();
  }

  auto frame_len = ntohl(*((uint32_t*) &connection->read_buf[4]));
  auto frame_len_full = frame_len + 8;
  if (connection->read_buf.size() < frame_len_full) {
    return ReturnCode::success();
  }

  auto rc = handleFrame(
      connection,
      ntohs(*((uint16_t*) &connection->read_buf[0])),
      ntohs(*((uint16_t*) &connection->read_buf[2])),
      connection->read_buf.data() + 8,
      frame_len);

  if (!rc.isSuccess()) {
    return rc;
  }

  if (connection->read_buf.size() == frame_len_full) {
    connection->read_buf.clear();
  } else {
    auto rem = connection->read_buf.size() - frame_len_full;
    memmove(
        &connection->read_buf[0],
        &connection->read_buf[frame_len_full],
        rem);

    connection->read_buf.resize(rem);
  }

  return ReturnCode::success();
}

ReturnCode AggregationScheduler::performWrite(Connection* connection) {
  if (connection->state == ConnectionState::CONNECTING) {
    connection->state = ConnectionState::CONNECTED;
    connection->read_timeout = MonotonicClock::now() + idle_timeout_;
    return handleHandshake(connection);
  }

  assert(connection->write_buf.size() > 0);
  int ret = ::write(
      connection->fd,
      connection->write_buf.data() + connection->write_buf_pos,
      connection->write_buf.size() - connection->write_buf_pos);

  if (ret == -1) {
    if (ret == EAGAIN) {
      return ReturnCode::success();
    }

    return ReturnCode::error(
        "EIO",
        "write() failed: %s",
        strerror(errno));
  }

  connection->write_buf_pos += ret;
  if (connection->write_buf_pos >= connection->write_buf.size()) {
    connection->write_buf.clear();
    connection->write_buf_pos = 0;
    connection->write_timeout = 0;
  } else {
    connection->write_timeout = MonotonicClock::now() + io_timeout_;
  }

  return ReturnCode::success();
}

ReturnCode AggregationScheduler::startNextPart() {
  auto iter = runq_.begin();

  while (iter != runq_.end()) {
    const auto& iter_host = (*iter)->hosts.front();
    if (connections_per_host_[iter_host] < max_concurrent_tasks_per_host_) {
      break;
    } else {
      ++iter;
    }
  }

  if (iter == runq_.end()) {
    return ReturnCode::success();
  }

  assert(
      (*iter)->state == AggregationPartState::INIT ||
      (*iter)->state == AggregationPartState::RETRY);

  auto part = *iter;
  runq_.erase(iter);
  ++num_parts_running_;

  part->state = AggregationPartState::RUNNING;
  auto rc = startConnection(part);
  if (!rc.isSuccess()) {
    rc = failPart(part);
  }

  return rc;
}

ReturnCode AggregationScheduler::failPart(AggregationPart* part) {
  while (part->hosts.size() > 1) {
    part->hosts.erase(part->hosts.begin());
    part->state = AggregationPartState::RETRY;

    const auto& part_host = part->hosts.front();
    if (connections_per_host_[part_host] >= max_concurrent_tasks_per_host_) {
      --num_parts_running_;
      runq_.emplace_back(part);
      return ReturnCode::success();
    }

    auto rc = startConnection(part);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  delete part;
  ++num_parts_complete_;
  --num_parts_running_;

  auto tolerate_failures = true;
  if (tolerate_failures) {
    return ReturnCode::success();
  } else {
    return ReturnCode::error("ERUNTIME", "aggregation failed");
  }
}

ReturnCode AggregationScheduler::startConnection(AggregationPart* part) {
  Connection connection;
  connection.host = part->hosts[0];
  connection.part = part;
  connection.write_buf_pos = 0;
  connection.state = ConnectionState::CONNECTING;
  connection.write_timeout = MonotonicClock::now() + io_timeout_;
  connection.read_timeout = 0;

  auto server_cfg = config_->getServerConfig(connection.host);
  if (server_cfg.server_status() != SERVER_UP) {
    return ReturnCode::error("ERUNTIME", "server is down");
  }

  auto server_addr = InetAddr::resolve(server_cfg.server_addr()); // FIXME
  auto server_ip = server_addr.ip();

  logDebug("evql", "Opening connection to $0", connection.host);
  auto fd = connection.fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    return ReturnCode::error(
        "EIO",
        "socket() creation failed: %s",
        strerror(errno));
  }

  if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) != 0) {
    close(fd);
    return ReturnCode::error(
        "EIO",
        "fcntl() failed: %s",
        strerror(errno));
  }

  struct sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(server_addr.port());
  inet_aton(server_ip.c_str(), &(saddr.sin_addr));
  memset(&(saddr.sin_zero), 0, 8);

  if (::connect(fd, (const struct sockaddr *) &saddr, sizeof(saddr)) < 0 &&
      errno != EINPROGRESS) {
    close(fd);
    return ReturnCode::error(
        "EIO",
        "connect() failed: %s",
        strerror(errno));
  }

  ++connections_per_host_[connection.host];
  connections_.push_back(connection);
  return ReturnCode::success();
}

void AggregationScheduler::closeConnection(Connection* connection) {
  --connections_per_host_[connection->host];
  ::close(connection->fd);
}

} // namespace eventql

