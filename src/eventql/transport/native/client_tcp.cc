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
#include <poll.h>
#include <sys/resource.h>
#include "eventql/util/logging.h"
#include "eventql/util/wallclock.h"
#include "eventql/util/util/binarymessagereader.h"
#include "eventql/transport/native/client_tcp.h"
#include "eventql/transport/native/frames/hello.h"
#include "eventql/transport/native/frames/error.h"

namespace eventql {
namespace native_transport {

TCPClient::TCPClient(
    TCPConnectionPool* conn_pool,
    net::DNSCache* dns_cache,
    uint64_t io_timeout /* = kDefaultIOTimeout */,
    uint64_t idle_timeout /* = kDefaultIdleTimeout */) :
    conn_pool_(conn_pool),
    dns_cache_(dns_cache),
    io_timeout_(io_timeout),
    idle_timeout_(idle_timeout) {}

TCPClient::~TCPClient() {
  close();
}

ReturnCode TCPClient::connect(
    const std::string& host,
    uint64_t port,
    bool is_internal,
    const AuthDataType& auth_data /* = AuthDataType{} */) {
  return connect(
      StringUtil::format("$0:$1", host, port),
      is_internal,
      auth_data);
}

ReturnCode TCPClient::connect(
    const std::string& addr_str,
    bool is_internal,
    const AuthDataType& auth_data /* = AuthDataType{} */) {
  if (conn_) {
    close();
  }

  if (is_internal &&
      conn_pool_ &&
      conn_pool_->getConnection(addr_str, &conn_)) {
    return ReturnCode::success();
  }

  InetAddr server_addr;
  if (dns_cache_) {
    server_addr = dns_cache_->resolve(addr_str);
  } else {
    server_addr = InetAddr::resolve(addr_str);
  }

  auto server_ip = server_addr.ip();

  logDebug("evql", "Opening connection to $0", addr_str);
  auto fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    return ReturnCode::error(
        "EIO",
        "socket() creation failed: %s",
        strerror(errno));
  }

  if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) != 0) {
    ::close(fd);
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
    ::close(fd);
    return ReturnCode::error(
        "EIO",
        "connect() failed: %s",
        strerror(errno));
  }

  struct pollfd p;
  p.fd = fd;
  p.events = POLLOUT;
  int poll_rc = poll(&p, 1, io_timeout_ / 1000);
  switch (poll_rc) {
    case -1:
      if (errno == EAGAIN || errno == EINTR) {
        /* fallthrough */
      } else {
        ::close(fd);
        return ReturnCode::error("EIO", "poll() failed: %s", strerror(errno));
      }
    case 0:
      ::close(fd);
      return ReturnCode::error("EIO", "operation timed out");
  }

  conn_.reset(new TCPConnection(fd, addr_str, is_internal, io_timeout_));

  auto handshake_rc = performHandshake(is_internal, auth_data);
  if (!handshake_rc.isSuccess()) {
    conn_->close();
    close();
  }

  return handshake_rc;
}

ReturnCode TCPClient::performHandshake(
    bool is_internal,
    const AuthDataType& auth_data) {
  native_transport::HelloFrame f_hello;
  f_hello.setIsInternal(is_internal);
  f_hello.setInteractiveAuth(false);
  f_hello.setIdleTimeout(idle_timeout_);
  f_hello.setAuthData(auth_data);

  auto rc = sendFrame(&f_hello, 0);
  if (!rc.isSuccess()) {
    return rc;
  }

  uint16_t res_opcode;
  uint16_t res_flags;
  std::string res_payload;
  rc = recvFrame(&res_opcode, &res_flags, &res_payload, io_timeout_);
  if (!rc.isSuccess()) {
    return rc;
  }

  switch (res_opcode) {
    case EVQL_OP_READY:
      break;
    case EVQL_OP_ERROR: {
      ErrorFrame error;
      error.parseFrom(res_payload.data(), res_payload.size());
      return ReturnCode::error("ERUNTIME", error.getError());
    }
    default:
      return ReturnCode::error("ERUNTIME", "invalid opcide");
  }

  return ReturnCode::success();
}

ReturnCode TCPClient::recvFrame(
    uint16_t* opcode,
    uint16_t* flags,
    std::string* payload,
    uint64_t timeout_us) {
  if (!conn_.get()) {
    return ReturnCode::error("ERUNTIME", "not connected");
  }

  return conn_->recvFrame(opcode, flags, payload, timeout_us);
}

ReturnCode TCPClient::sendFrame(
    uint16_t opcode,
    uint16_t flags,
    const void* payload,
    size_t payload_len) {
  if (!conn_.get()) {
    return ReturnCode::error("ERUNTIME", "not connected");
  }

  return conn_->sendFrame(opcode, flags, payload, payload_len);
}

void TCPClient::close() {
  if (!conn_) {
    return;
  }

  if (conn_pool_) {
    conn_pool_->storeConnection(std::move(conn_));
  } else {
    conn_->close();
    conn_.reset(nullptr);
  }
}

TCPAsyncClient::TCPAsyncClient(
    ProcessConfig* config,
    ConfigDirectory* config_dir,
    TCPConnectionPool* conn_pool,
    net::DNSCache* dns_cache,
    size_t max_concurrent_tasks,
    size_t max_concurrent_tasks_per_host,
    bool tolerate_failures) :
    config_(config_dir),
    conn_pool_(conn_pool),
    dns_cache_(dns_cache),
    max_concurrent_tasks_(max_concurrent_tasks),
    max_concurrent_tasks_per_host_(max_concurrent_tasks_per_host),
    tolerate_failures_(tolerate_failures),
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
  task->started = false;
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

  if (!connection->task->started) {
    if (rpc_started_cb_) {
      rpc_started_cb_(connection->task->privdata);
    }

    connection->task->started = true;
  }

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
    completeTask(task, true);
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

    if (!connection->task->started) {
      if (rpc_started_cb_) {
        rpc_started_cb_(connection->task->privdata);
      }

      connection->task->started = true;
    }
  } else {
    connection->state = ConnectionState::CLOSE;
  }

  return ReturnCode::success();
}

ReturnCode TCPAsyncClient::execute() {
  if (num_tasks_ == 0) {
    return ReturnCode::success();
  }

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
        auto task = conn->task;
        closeConnection(&*conn, false);
        conn = connections_.erase(conn);
        auto rc = failTask(
            task,
            ReturnCode::error("EIO", "Client connection timeout out"));
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
          auto task = conn->task;
          closeConnection(&*conn, false);
          conn = connections_.erase(conn);
          rc = failTask(task, rc);
          if (rc.isSuccess()) {
            continue;
          } else {
            shutdown();
            return rc;
          }
        }

        if (conn->state == ConnectionState::CLOSE) {
          closeConnection(&*conn, true);
          conn = connections_.erase(conn);
          continue;
        }
      }

      if (FD_ISSET(conn->fd, &op_write)) {
        conn->write_timeout = 0;

        auto rc = performWrite(&*conn);
        if (!rc.isSuccess()) {
          auto task = conn->task;
          closeConnection(&*conn, false);
          conn = connections_.erase(conn);
          rc = failTask(task, rc);
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
    closeConnection(&*connections_iter, true);
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
    if (eof) {
      return ReturnCode::error("EIO", "connection to server lost");
    } else {
      return ReturnCode::success(); // wait for more data
    }
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
    rc = failTask(task, rc);
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

  return task;
}

ReturnCode TCPAsyncClient::failTask(Task* task, const ReturnCode& fail_rc) {
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
      logError("evqld", "Client error: $0", rc.getMessage());
    }
  }

  completeTask(task, false);

  if (tolerate_failures_) {
    logError("evqld", "Client error: $0", fail_rc.getMessage());
    return ReturnCode::success();
  } else {
    return fail_rc;
  }
}

void TCPAsyncClient::completeTask(Task* task, bool success) {
  if (rpc_completed_cb_) {
    rpc_completed_cb_(task->privdata, success);
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
  connection.fd = -1;

  auto server_cfg = config_->getServerConfig(connection.host);
  if (server_cfg.server_status() != SERVER_UP) {
    return ReturnCode::error("ERUNTIME", "server is down");
  }

  connection.host_addr = server_cfg.server_addr();

  if (conn_pool_) {
    connection.fd = conn_pool_->getFD(server_cfg.server_addr());
    if (connection.fd >= 0) {
      connection.state = ConnectionState::READY;
    }
  }

  if (connection.fd < 0) {
    InetAddr server_addr;
    if (dns_cache_) {
      server_addr = dns_cache_->resolve(server_cfg.server_addr());
    } else {
      server_addr = InetAddr::resolve(server_cfg.server_addr());
    }

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
  }

  ++connections_per_host_[connection.host];
  connections_.push_back(connection);

  if (connection.state == ConnectionState::READY) {
    return handleReady(&connections_.back());
  } else {
    return ReturnCode::success();
  }
}

void TCPAsyncClient::closeConnection(Connection* connection, bool graceful) {
  --connections_per_host_[connection->host];

  if (graceful && conn_pool_) {
    conn_pool_->storeFD(connection->fd, connection->host_addr);
  } else {
    ::close(connection->fd);
  }
}

TCPConnectionPool::TCPConnectionPool(
    uint64_t max_conns,
    uint64_t max_conns_per_host,
    uint64_t max_conn_age,
    uint64_t io_timeout) :
    max_conns_(max_conns),
    max_conns_per_host_(max_conns_per_host),
    max_conn_age_(max_conn_age),
    io_timeout_(io_timeout),
    num_conns_(0) {
  struct rlimit fd_limit;
  memset(&fd_limit, 0, sizeof(fd_limit));
  ::getrlimit(RLIMIT_NOFILE, &fd_limit);

  static const size_t kReservedFDs = 64;
  size_t maxfds = fd_limit.rlim_cur;
  if (maxfds > kReservedFDs) {
    maxfds -= kReservedFDs;
  } else {
    maxfds = 0;
  }

  if (maxfds < max_conns_) {
    logWarning(
        "evlqd",
        "RLIMIT_NOFILE is too small ($0), reducing maximum connection pool "
        "size from $1 to $2. Consider increasing the file descriptor limit.",
        fd_limit.rlim_cur,
        max_conns_,
        maxfds);

    max_conns_ = maxfds;
  }
}

bool TCPConnectionPool::getConnection(
    const std::string& addr,
    std::unique_ptr<TCPConnection>* connection) {
  auto fd = getFD(addr);
  if (fd >= 0) {
    connection->reset(
        new TCPConnection(
            fd,
            addr,
            true,
            io_timeout_));

    return true;
  } else {
    return false;
  }
}

int TCPConnectionPool::getFD(const std::string& server) {
  auto now = MonotonicClock::now();
  auto cutoff = now;
  if (cutoff > max_conn_age_) {
    cutoff -= max_conn_age_;
  } else {
    cutoff = 0;
  }

  std::unique_lock<std::mutex> lk(mutex_);
  auto iter = conns_.find(server);
  if (iter == conns_.end()) {
    return -1;
  }

  auto& conns = iter->second;
  for (size_t i = conns.size(); i-- > 0; ) {
    if (conns[i].time > cutoff) {
      auto fd = conns[i].fd;
      conns.erase(conns.begin() + i);
      --num_conns_;
      return fd;
    }
  }

  return -1;
}

void TCPConnectionPool::storeConnection(
    std::unique_ptr<TCPConnection>&& connection) {
  auto c = std::move(connection);

  if (c->isClosed() || !c->isInternal()) {
    return;
  }

  storeFD(c->releaseFD(), c->getRemoteHost());
}

void TCPConnectionPool::storeFD(
    int fd,
    const std::string& server) {
  auto now = MonotonicClock::now();
  auto cutoff = now;
  if (cutoff > max_conn_age_) {
    cutoff -= max_conn_age_;
  } else {
    cutoff = 0;
  }

  if (fd < 0) {
    return;
  }

  std::set<int> close_fds;
  {
    std::unique_lock<std::mutex> lk(mutex_);
    if (num_conns_ >= max_conns_) {
      lk.unlock();
      close(fd);
      return;
    }

    auto& connlist = conns_[server];
    while (
        connlist.size() >= max_conns_per_host_ ||
        (!connlist.empty() && connlist.back().time < cutoff)) {
      close_fds.insert(connlist.back().fd);
      connlist.pop_back();
      --num_conns_;
    }

    CachedConnection c;
    c.fd = fd;
    c.time = now;
    connlist.insert(connlist.begin(), c);
    ++num_conns_;
  }

  for (const auto& close_fd : close_fds) {
    close(close_fd);
  }
}

} // namespace native_transport
} // namespace eventql

