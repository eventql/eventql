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
#include "eventql/util/util/binarymessagereader.h"
#include "eventql/transport/native/client_tcp.h"
#include "eventql/transport/native/frames/hello.h"
#include "eventql/transport/native/frames/error.h"

namespace eventql {
namespace native_transport {

TCPAsyncClient::TCPAsyncClient(
    ProcessConfig* config,
    ConfigDirectory* config_dir,
    size_t max_concurrent_tasks,
    size_t max_concurrent_tasks_per_host) :
    config_(config_dir),
    max_concurrent_tasks_(max_concurrent_tasks),
    max_concurrent_tasks_per_host_(max_concurrent_tasks_per_host),
    num_tasks_(0),
    num_tasks_complete_(0),
    num_tasks_running_(0),
    io_timeout_(config->getInt("server.s2s_io_timeout").get()),
    idle_timeout_(config->getInt("server.s2s_idle_timeout").get()) {}

TCPAsyncClient::~TCPAsyncClient() {
  shutdown();
}

void TCPAsyncClient::addRPC(
    uint16_t opcode,
    uint16_t flags,
    std::string&& payload,
    const std::vector<std::string>& hosts,
    void* privdata /* = nullptr */) {
  assert(hosts.size() > 0);
  auto task = new Task();
  task->opcode = opcode;
  task->flags = flags;
  task->payload = std::move(payload);
  task->hosts = hosts;
  task->privdata = privdata;
  runq_.push_back(task);
  ++num_tasks_;
}

void TCPAsyncClient::setResultCallback(ResultCallbackType fn) {
  result_cb_ = fn;
}

void TCPAsyncClient::setRPCStartedCallback(RPCStartedCallbackType fn) {
  rpc_started_cb_ = fn;
}

void TCPAsyncClient::setRPCCompletedCallback(RPCCompletedCallbackType fn) {
  rpc_completed_cb_ = fn;
}

ReturnCode TCPAsyncClient::handleFrame(
    Connection* connection,
    uint16_t opcode,
    uint16_t flags,
    const char* payload,
    size_t payload_size) {
  if (opcode == EVQL_OP_ERROR) {
    ErrorFrame error;
    error.parseFrom(payload, payload_size);
    return ReturnCode::error("ERUNTIME", error.getError());
  }

  switch (connection->state) {

    case ConnectionState::HANDSHAKE:
      switch (opcode) {
        case EVQL_OP_READY:
          connection->state = ConnectionState::READY;
          return handleReady(connection);
        default:
          return ReturnCode::error("ERUNTIME", "unexpected opcode");
      }
      break;

    case ConnectionState::QUERY:
      return handleResult(connection, opcode, flags, payload, payload_size);

    default:
      break;

  }

  return ReturnCode::error("ERUNTIME", "unexpected opcode");
}

void TCPAsyncClient::sendFrame(
    Connection* connection,
    uint16_t opcode,
    uint16_t flags,
    const char* payload,
    size_t payload_len) {
  auto os = StringOutputStream::fromString(&connection->write_buf);
  os->appendNUInt16(opcode);
  os->appendNUInt16(flags);
  os->appendNUInt32(payload_len);
  os->write(payload, payload_len);
}

ReturnCode TCPAsyncClient::handleReady(Connection* connection) {
  logDebug("evqld", "Executing RPC on $0", connection->host);
  connection->state = ConnectionState::QUERY;
  sendFrame(
      connection,
      connection->task->opcode,
      connection->task->flags,
      connection->task->payload.data(),
      connection->task->payload.size());

  return ReturnCode::success();
}

ReturnCode TCPAsyncClient::handleResult(
    Connection* connection,
    uint16_t opcode,
    uint16_t flags,
    const char* payload,
    size_t payload_size) {
  logDebug("evqld", "Received RPC result from $0", connection->host);

  auto task = connection->task;
  if (result_cb_) {
    auto rc = result_cb_(
        task->privdata,
        opcode,
        flags,
        payload,
        payload_size);

    if (!rc.isSuccess()) {
      return rc;
    }
  }

  if (flags & EVQL_ENDOFREQUEST) {
    completeTask(task);
    connection->task = nullptr;
    return handleIdle(connection);
  } else {
    return ReturnCode::success();
  }
}

ReturnCode TCPAsyncClient::handleHandshake(Connection* connection) {
  connection->state = ConnectionState::HANDSHAKE;

  std::string payload;
  auto payload_os = StringOutputStream::fromString(&payload);

  native_transport::HelloFrame f_hello;
  f_hello.setIsInternal(true);
  f_hello.setIdleTimeout(idle_timeout_);
  f_hello.writeTo(payload_os.get());

  sendFrame(
      connection,
      EVQL_OP_HELLO,
      0,
      payload.data(),
      payload.size());

  return ReturnCode::success();
}

ReturnCode TCPAsyncClient::handleIdle(Connection* connection) {
  auto next_task = popTask(&connection->host); // get next task
  if (next_task) {
    connection->state = ConnectionState::QUERY;
    connection->task = next_task;
    sendFrame(
        connection,
        connection->task->opcode,
        connection->task->flags,
        connection->task->payload.data(),
        connection->task->payload.size());
  } else {
    connection->state = ConnectionState::CLOSE;
  }

  return ReturnCode::success();
}

ReturnCode TCPAsyncClient::execute() {
  fd_set op_read, op_write, op_error;
  while (num_tasks_complete_ < num_tasks_) {
    for (size_t i = num_tasks_running_; i < max_concurrent_tasks_; ++i) {
      auto rc = startNextTask();
      if (!rc.isSuccess()) {
        shutdown();
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
      shutdown();
      return ReturnCode::error(
          "EIO",
          "select() failed: %s",
          strerror(errno));
    }

    now = MonotonicClock::now();
    for (auto conn = connections_.begin(); conn != connections_.end(); ) {
      if ((conn->read_timeout > 0 && conn->read_timeout <= now) ||
          (conn->write_timeout > 0 && conn->write_timeout <= now)) {
        logDebug("evqld", "Client connection timed out");

        auto task = conn->task;
        closeConnection(&*conn);
        conn = connections_.erase(conn);
        auto rc = failTask(task);
        if (rc.isSuccess()) {
          continue;
        } else {
          shutdown();
          return rc;
        }
      }

      if (FD_ISSET(conn->fd, &op_read)) {
        conn->read_timeout = MonotonicClock::now() + idle_timeout_;

        auto rc = performRead(&*conn);
        if (!rc.isSuccess()) {
          logDebug("evqld", "Client error: $0", rc.getMessage());
          auto task = conn->task;
          closeConnection(&*conn);
          conn = connections_.erase(conn);
          rc = failTask(task);
          if (rc.isSuccess()) {
            continue;
          } else {
            shutdown();
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
          logDebug("evqld", "Client error: $0", rc.getMessage());
          auto task = conn->task;
          closeConnection(&*conn);
          conn = connections_.erase(conn);
          rc = failTask(task);
          if (rc.isSuccess()) {
            continue;
          } else {
            shutdown();
            return rc;
          }
        }
      }

      ++conn;
    }
  }

  shutdown();
  return ReturnCode::success();
}

void TCPAsyncClient::shutdown() {
  auto runq_iter = runq_.begin();
  while (runq_iter != runq_.end()) {
    delete *runq_iter;
    runq_iter = runq_.erase(runq_iter);
  }

  auto connections_iter = connections_.begin();
  while (connections_iter != connections_.end()) {
    closeConnection(&*connections_iter);
    connections_iter = connections_.erase(connections_iter);
  }
}

ReturnCode TCPAsyncClient::performRead(Connection* connection) {
  size_t batch_size = 4096;
  bool eof = false;

  for (int ret = 1; ret > 0; ) {
    auto begin = connection->read_buf.size();
    connection->read_buf.resize(begin + batch_size);

    ret = ::read(
        connection->fd,
        (void*) (&connection->read_buf[0] + begin),
        batch_size);

    switch (ret) {

      case -1:
        if (errno == EAGAIN || errno == EINTR) {
          connection->read_buf.resize(begin);
          break;
        } else {
          return ReturnCode::error(
              "EIO",
              "read() failed: %s",
              strerror(errno));
        }

      case 0:
        eof = true;
        /* fallthrough */

      default:
        connection->read_buf.resize(begin + ret);
        break;

    }
  }

  if (connection->read_buf.size() < 8) {
    return ReturnCode::success();
  }

  auto frame_len = ntohl(*((uint32_t*) &connection->read_buf[4]));
  auto frame_len_full = frame_len + 8;
  if (connection->read_buf.size() < frame_len_full) {
    if (eof) {
      return ReturnCode::error("EIO", "connection to server lost");
    } else {
      return ReturnCode::success(); // wait for more data
    }
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

ReturnCode TCPAsyncClient::performWrite(Connection* connection) {
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
    if (errno == EAGAIN) {
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

ReturnCode TCPAsyncClient::startNextTask() {
  auto task = popTask();
  if (!task) {
    return ReturnCode::success();
  }

  auto rc = startConnection(task);
  if (!rc.isSuccess()) {
    logDebug("evqld", "Client error: $0", rc.getMessage());
    rc = failTask(task);
  }

  return rc;
}

TCPAsyncClient::Task* TCPAsyncClient::popTask(
    const std::string* host /* = nullptr */) {
  auto iter = runq_.begin();

  while (iter != runq_.end()) {
    const auto& iter_host = (*iter)->hosts.front();
    if (host && iter_host != *host) {
      ++iter;
      continue;
    }

    if (connections_per_host_[iter_host] >= max_concurrent_tasks_per_host_) {
      ++iter;
      continue;
    }

    break;
  }

  if (iter == runq_.end()) {
    return nullptr;
  }

  auto task = *iter;
  runq_.erase(iter);
  ++num_tasks_running_;

  if (rpc_started_cb_) {
    rpc_started_cb_(task->privdata);
  }

  return task;
}

ReturnCode TCPAsyncClient::failTask(Task* task) {
  while (task->hosts.size() > 1) {
    task->hosts.erase(task->hosts.begin());

    const auto& task_host = task->hosts.front();
    if (connections_per_host_[task_host] >= max_concurrent_tasks_per_host_) {
      --num_tasks_running_;
      runq_.emplace_back(task);
      return ReturnCode::success();
    }

    auto rc = startConnection(task);
    if (rc.isSuccess()) {
      return rc;
    } else {
      logDebug("evqld", "Client error: $0", rc.getMessage());
    }
  }

  completeTask(task);

  auto tolerate_failures = true;
  if (tolerate_failures) {
    return ReturnCode::success();
  } else {
    return ReturnCode::error("ERUNTIME", "aggregation failed");
  }
}

void TCPAsyncClient::completeTask(Task* task) {
  if (rpc_completed_cb_) {
    rpc_completed_cb_(task->privdata);
  }

  delete task;
  ++num_tasks_complete_;
  --num_tasks_running_;
}

ReturnCode TCPAsyncClient::startConnection(Task* task) {
  Connection connection;
  connection.host = task->hosts[0];
  connection.task = task;
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

void TCPAsyncClient::closeConnection(Connection* connection) {
  --connections_per_host_[connection->host];
  ::close(connection->fd);
}

} // namespace native_transport
} // namespace eventql

