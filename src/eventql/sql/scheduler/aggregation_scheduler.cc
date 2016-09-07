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
#include "eventql/sql/scheduler/aggregation_scheduler.h"

namespace eventql {

AggregationScheduler::AggregationScheduler(
    ConfigDirectory* config,
    size_t max_concurrent_tasks,
    size_t max_concurrent_tasks_per_host) :
    config_(config),
    max_concurrent_tasks_(max_concurrent_tasks),
    max_concurrent_tasks_per_host_(max_concurrent_tasks_per_host) {}

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
}

void AggregationScheduler::setResultCallback(
    const std::function<void ()> fn) {

}

ReturnCode AggregationScheduler::execute() {
  for (size_t i = 0; i < max_concurrent_tasks_; ++i) {
    auto rc = startNextPart();
    if (!rc.isSuccess()) {
      return rc; // FIXME check tolerate failing shards
    }
  }

  while (true) {
    fd_set op_read, op_write, op_error;
    int max_fd = 0;
    FD_ZERO(&op_read);
    FD_ZERO(&op_error);
    FD_ZERO(&op_write);

    for (const auto& connection : connections_) {
      if (connection.needs_read) {
        FD_SET(connection.fd, &op_read);
        max_fd = std::max(max_fd, connection.fd);
      }

      if (connection.needs_write) {
        FD_SET(connection.fd, &op_write);
        max_fd = std::max(max_fd, connection.fd);
      }
    }

    int res = select(max_fd + 1, &op_read, &op_write, &op_error, NULL);
    if (res == -1 && errno != EINTR) {
      return ReturnCode::error(
          "EIO",
          "select() failed: %s",
          strerror(errno));
    }

    for (auto conn = connections_.begin(); conn != connections_.end(); ) {
      if (conn->needs_read && FD_ISSET(conn->fd, &op_read)) {
        conn->needs_read = false;

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

      if (conn->needs_write && FD_ISSET(conn->fd, &op_write)) {
        conn->needs_write = false;

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
  logDebug("evql", "perform read");
  return ReturnCode::success();
}

ReturnCode AggregationScheduler::performWrite(Connection* connection) {
  logDebug("evql", "perform write");
  if (connection->state == ConnectionState::CONNECTING) {
    return startConnectionHandshake(connection);
  }

  assert(connection->write_buf.size() > 0);
  int ret = ::write(
      connection->fd,
      (const char*) connection->write_buf.data() + connection->write_buf.mark(),
      connection->write_buf.size() - connection->write_buf.mark());

  if (ret == -1) {
    if (ret == EAGAIN) {
      connection->needs_write = true;
      return ReturnCode::success();
    }

    return ReturnCode::error(
        "EIO",
        "write() failed: %s",
        strerror(errno));
  }

  connection->write_buf.setMark(connection->write_buf.mark() + ret);
  if (connection->write_buf.mark() == connection->write_buf.size()) {
    connection->write_buf.clear();
  } else {
    connection->needs_write = true;
  }

  return ReturnCode::success();
}

ReturnCode AggregationScheduler::startNextPart() {
  auto iter = runq_.begin();
  if (iter == runq_.end()) {
    return ReturnCode::success();
  }

  assert(
      (*iter)->state == AggregationPartState::INIT ||
      (*iter)->state == AggregationPartState::RETRY);

  auto part = *iter;
  runq_.erase(iter);

  auto rc = startConnection(part);
  if (!rc.isSuccess()) {
    rc = failPart(part);
  }

  return rc;
}

ReturnCode AggregationScheduler::failPart(AggregationPart* part) {
  part->hosts.erase(part->hosts.begin());

  while (!part->hosts.empty()) {
    part->state = AggregationPartState::RETRY;
    auto rc = startConnection(part);
    if (!rc.isSuccess()) {
      return rc;
    } else {
      part->hosts.erase(part->hosts.begin());
    }
  }

  delete part;

  auto tolerate_failures = true;
  if (tolerate_failures) {
    return ReturnCode::success();
  } else {
    return ReturnCode::error("ERUNTIME", "aggregation failed");
  }
}

ReturnCode AggregationScheduler::startConnection(AggregationPart* part) {
  Connection connection;
  connection.state = ConnectionState::INIT;
  connection.host = part->hosts[0];

  part->state = AggregationPartState::RUNNING;

  auto server_cfg = config_->getServerConfig(connection.host);
  if (server_cfg.server_status() != SERVER_UP) {
    return ReturnCode::error("ERUNTIME", "server is down");
  }

  auto server_addr = InetAddr::resolve(server_cfg.server_addr());
  auto server_ip = server_addr.ip();

  logDebug("evql", "Opening connection to $0", connection.host);
  int fd = socket(AF_INET, SOCK_STREAM, 0);
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

  connection.fd = fd;
  connection.needs_write = true;
  connection.needs_read = false;
  connection.state = ConnectionState::CONNECTING;
  connections_.push_back(connection);
  return ReturnCode::success();
}

ReturnCode AggregationScheduler::startConnectionHandshake(
    Connection* connection) {
  logDebug("evql", "start connection handshake");
  return ReturnCode::success();
}

void AggregationScheduler::closeConnection(Connection* connection) {
  ::close(connection->fd);
}

//void sendFrame();
//void recvFrame();

} // namespace eventql


