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
#pragma once
#include "eventql/eventql.h"
#include "eventql/sql/qtree/GroupByNode.h"
#include "eventql/util/return_code.h"
#include "eventql/util/buffer.h"
#include "eventql/config/config_directory.h"
#include "eventql/transport/native/connection_tcp.h"

namespace eventql {
namespace native_transport {

// A TCP Client is _not_ thread safe
class TCPClient {
public:

  using ResultCallbackType =
      std::function<
          ReturnCode (
              void* privdata,
              uint16_t opcode,
              uint16_t flags,
              const char* payload,
              size_t payload_len)>;

  TCPClient(
      ProcessConfig* config,
      ConfigDirectory* config_dir);

  ReturnCode connect(const std::string& server);

  ReturnCode recvFrame(
      uint16_t* opcode,
      uint16_t* flags,
      std::string* payload,
      uint64_t timeout_us);

  ReturnCode sendFrame(
      uint16_t opcode,
      uint16_t flags,
      const void* payload,
      size_t payload_len);

  template <class FrameType>
  ReturnCode sendFrame(
      const FrameType* frame,
      uint16_t flags);

  void close();

protected:

  ReturnCode performHandshake();

  ProcessConfig* config_;
  ConfigDirectory* cdir_;
  uint64_t io_timeout_;
  uint64_t idle_timeout_;
  std::unique_ptr<TCPConnection> conn_;
};

// A AsyncTCPClient is _not_ thread safe
class TCPAsyncClient {
public:

  using ResultCallbackType =
      std::function<
          ReturnCode (
              void* privdata,
              uint16_t opcode,
              uint16_t flags,
              const char* payload,
              size_t payload_len)>;

  using RPCStartedCallbackType = std::function<void (void* privdata)>;
  using RPCCompletedCallbackType = std::function<void (void* privdata)>;

  TCPAsyncClient(
      ProcessConfig* config,
      ConfigDirectory* config_dir,
      size_t max_concurrent_tasks,
      size_t max_concurrent_tasks_per_host,
      bool tolerate_failures);

  ~TCPAsyncClient();

  void addRPC(
      uint16_t opcode,
      uint16_t flags,
      std::string&& payload,
      const std::vector<std::string>& hosts,
      void* privdata = nullptr);

  void setResultCallback(ResultCallbackType fn);
  void setRPCStartedCallback(RPCStartedCallbackType fn);
  void setRPCCompletedCallback(RPCCompletedCallbackType fn);

  ReturnCode execute();
  void shutdown();

protected:

  struct Task {
    std::vector<std::string> hosts;
    uint16_t opcode;
    uint16_t flags;
    std::string payload;
    void* privdata;
  };

  enum class ConnectionState {
    CONNECTING, HANDSHAKE, CONNECTED, READY, QUERY, CLOSE
  };

  struct Connection {
    ConnectionState state;
    std::string host;
    int fd;
    std::string read_buf;
    std::string write_buf;
    size_t write_buf_pos;
    bool needs_write;
    bool needs_read;
    uint64_t read_timeout;
    uint64_t write_timeout;
    Task* task;
  };

  ReturnCode handleFrame(
      Connection* connection,
      uint16_t opcode,
      uint16_t flags,
      const char* payload,
      size_t payload_size);

  ReturnCode handleHandshake(Connection* connection);
  ReturnCode handleReady(Connection* connection);
  ReturnCode handleIdle(Connection* connection);
  ReturnCode handleResult(
      Connection* connection,
      uint16_t opcode,
      uint16_t flags,
      const char* payload,
      size_t payload_size);

  Task* popTask(const std::string* hostname = nullptr);
  ReturnCode failTask(Task* task, const ReturnCode& fail_rc);
  void completeTask(Task* task);

  ReturnCode startNextTask();
  ReturnCode startConnection(Task* task);
  void closeConnection(Connection* connection);
  ReturnCode performWrite(Connection* connection);
  ReturnCode performRead(Connection* connection);
  void sendFrame(
      Connection* connection,
      uint16_t opcode,
      uint16_t flags,
      const char* payload,
      size_t payload_len);

  ResultCallbackType result_cb_;
  RPCStartedCallbackType rpc_started_cb_;
  RPCCompletedCallbackType rpc_completed_cb_;
  std::deque<Task*> runq_;
  std::list<Connection> connections_;
  ConfigDirectory* config_;
  std::map<std::string, size_t> connections_per_host_;
  size_t max_concurrent_tasks_;
  size_t max_concurrent_tasks_per_host_;
  bool tolerate_failures_;
  size_t num_tasks_;
  size_t num_tasks_complete_;
  size_t num_tasks_running_;
  size_t io_timeout_;
  size_t idle_timeout_;
  bool tolerate_failed_shards_;
};

} // namespace native_transport
} // namespace eventql

#include "client_tcp_impl.h"
