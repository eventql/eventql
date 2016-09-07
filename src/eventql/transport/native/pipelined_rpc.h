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

namespace eventql {

class PipelinedRPC {
public:

  PipelinedRPC(
      ConfigDirectory* config,
      size_t max_concurrent_tasks,
      size_t max_concurrent_tasks_per_host);

  ~PipelinedRPC();

  void addLocalPart(
      const csql::GroupByNode* query);

  void addRemotePart(
      const csql::GroupByNode* query,
      const std::vector<std::string>& hosts);

  void setResultCallback(
      const std::function<void ()> fn);

  ReturnCode execute();
  void shutdown();

protected:

  enum class AggregationPartState {
    INIT, RUNNING, RETRY, DONE
  };

  struct AggregationPart {
    AggregationPartState state;
    std::vector<std::string> hosts;
  };

  enum class ConnectionState {
    CONNECTING, HANDSHAKE, CONNECTED, READY, QUERY, IDLE, CLOSE
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
    AggregationPart* part;
  };

  ReturnCode handleFrame(
      Connection* connection,
      uint16_t opcode,
      uint16_t flags,
      const char* payload,
      size_t payload_size);

  ReturnCode handleReady(Connection* connection);
  ReturnCode handleHandshake(Connection* connection);
  ReturnCode startNextPart();
  ReturnCode startConnection(AggregationPart* part);
  ReturnCode failPart(AggregationPart* part);
  void completePart(AggregationPart* part);
  ReturnCode performWrite(Connection* connection);
  ReturnCode performRead(Connection* connection);
  void closeConnection(Connection* connection);

  std::deque<AggregationPart*> runq_;
  std::list<Connection> connections_;
  ConfigDirectory* config_;
  std::map<std::string, size_t> connections_per_host_;
  size_t max_concurrent_tasks_;
  size_t max_concurrent_tasks_per_host_;
  size_t num_parts_;
  size_t num_parts_complete_;
  size_t num_parts_running_;
  size_t io_timeout_;
  size_t idle_timeout_;
};

} // namespace eventql

