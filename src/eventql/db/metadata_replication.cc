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
#include "eventql/db/metadata_replication.h"
#include "eventql/transport/native/client_tcp.h"
#include "eventql/transport/native/frames/error.h"
#include "eventql/transport/native/frames/meta_getfile.h"
#include "eventql/util/application.h"

namespace eventql {

MetadataReplication::MetadataReplication(
    ConfigDirectory* cdir,
    ProcessConfig* config,
    const String& server_name,
    MetadataStore* store) :
    cdir_(cdir),
    config_(config),
    server_name_(server_name),
    metadata_store_(store) {
  cdir_->setTableConfigChangeCallback([this] (const TableDefinition& tbl) {
    applyTableConfigChange(tbl);
  });

  cdir_->listTables([this] (const TableDefinition& tbl) {
    applyTableConfigChange(tbl);
  });
}

MetadataReplication::~MetadataReplication() {
  if (running_) {
    stop();
  }
}

void MetadataReplication::applyTableConfigChange(const TableDefinition& cfg) {
  bool has_local_replica = false;
  Vector<String> servers;
  for (const auto& s : cfg.metadata_servers()) {
    if (s == server_name_) {
      has_local_replica = true;
    } else {
      servers.emplace_back(s);
    }
  }

  if (!has_local_replica) {
    return;
  }

  ReplicationJob job;
  job.db_namespace = cfg.customer();
  job.table_id = cfg.table_name();
  job.servers = servers;
  job.transaction_id = SHA1Hash(
      cfg.metadata_txnid().data(),
      cfg.metadata_txnid().size());

  queue_.insert(job, WallClock::now());
}

void MetadataReplication::replicateWithRetries(const ReplicationJob& job) {
  auto rc = Status::success();
  try {
    rc = replicate(job);
  } catch (const std::exception& e) {
    rc = Status(eRuntimeError, e.what());
  }

  if (!rc.isSuccess()) {
    logWarning("evqld", "metadata replication failed: $0", rc.message());

    // check that the transaction we are retrying to fetch is still the most
    // recent one
    auto table_cfg = cdir_->getTableConfig(job.db_namespace, job.table_id);
    SHA1Hash head_txnid(
        table_cfg.metadata_txnid().data(),
        table_cfg.metadata_txnid().size());

    if (head_txnid == job.transaction_id) {
      queue_.insert(job, WallClock::unixMicros() + kRetryDelayMicros);
    }
  }
}

Status MetadataReplication::replicate(const ReplicationJob& job) {
  bool has_file = metadata_store_->hasMetadataFile(
      job.db_namespace,
      job.table_id,
      job.transaction_id);
  if (has_file) {
    return Status::success();
  }

  auto idle_timeout = config_->getInt("server.s2s_idle_timeout", 0);
  auto io_timeout = config_->getInt("server.s2s_io_timeout", 0);

  native_transport::MetaGetfileFrame m_frame;
  m_frame.setDatabase(job.db_namespace);
  m_frame.setTable(job.table_id);
  m_frame.setTransactionID(job.transaction_id);

  for (const auto& s : job.servers) {
    auto server = cdir_->getServerConfig(s);
    if (server.server_status() != SERVER_UP) {
      logWarning("evqld", "metadata server is down: $0", s);
      continue;
    }

    native_transport::TCPClient client(io_timeout, idle_timeout);
    auto rc = client.connect(server.server_addr(), true);
    if (!rc.isSuccess()) {
      logWarning(
          "evqld",
          "can't connect to metadata server: $0",
          rc.getMessage());
      continue;
    }

    rc = client.sendFrame(&m_frame, 0);
    if (!rc.isSuccess()) {
      logWarning("evqld", "metadata request failed: $0", rc.getMessage());
      continue;
    }

    uint16_t ret_opcode = 0;
    uint16_t ret_flags;
    std::string ret_payload;
    rc = client.recvFrame(&ret_opcode, &ret_flags, &ret_payload, idle_timeout);
    if (!rc.isSuccess()) {
      logWarning("evqld", "metadata request failed: $0", rc.getMessage());
      continue;
    }

    switch (ret_opcode) {
      case EVQL_OP_META_GETFILE_RESULT:
        break;
      case EVQL_OP_ERROR: {
        native_transport::ErrorFrame eframe;
        eframe.parseFrom(ret_payload.data(), ret_payload.size());
        logWarning("evqld", "metadata request failed: $0", eframe.getError());
        continue;
      }
      default:
        logWarning("evqld", "metadata request failed: invalid opcode");
        continue;
    }


    MetadataFile file;
    auto is = StringInputStream::fromString(ret_payload);
    rc = file.decode(is.get());
    if (!rc.isSuccess()) {
      logWarning("evqld", "invalid metadata file: $0", rc.getMessage());
      continue;
    }

    return metadata_store_->storeMetadataFile(
        job.db_namespace,
        job.table_id,
        file);
  }

  return Status(eIOError, "no metadata server responded");
}

void MetadataReplication::start() {
  running_ = true;

  for (int i = 0; i < 4; ++i) {
    threads_.emplace_back([this] () {
      Application::setCurrentThreadName("evqld-metadatareplication");
      while (running_) {
        auto job = queue_.interruptiblePop();

        try {
          if (!job.isEmpty()) {
            replicateWithRetries(job.get());
          }
        } catch (const std::exception& e) {
          logWarning("evqld", "error in metadata replication: $0", e.what());
        }
      }
    });
  }
}

void MetadataReplication::stop() {
  if (!running_) {
    return;
  }

  running_ = false;
  queue_.wakeup();

  for (auto& t : threads_) {
    t.join();
  }
}


} // namespace eventql
