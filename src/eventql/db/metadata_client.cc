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
#include "eventql/db/metadata_client.h"
#include "eventql/transport/native/client_tcp.h"
#include "eventql/transport/native/frames/error.h"
#include "eventql/transport/native/frames/meta_getfile.h"
#include <eventql/util/logging.h>

namespace eventql {

MetadataClient::MetadataClient(
    ConfigDirectory* cdir,
    ProcessConfig* config,
    MetadataStore* store,
    MetadataCache* cache,
    native_transport::TCPConnectionPool* conn_pool,
    net::DNSCache* dns_cache) :
    cdir_(cdir),
    config_(config),
    cache_(cache),
    conn_pool_(conn_pool),
    dns_cache_(dns_cache) {}

Status MetadataClient::fetchLatestMetadataFile(
    const String& ns,
    const String& table_id,
    RefPtr<MetadataFile>* file) {
  auto table_cfg = cdir_->getTableConfig(ns, table_id);

  SHA1Hash txid(
      table_cfg.metadata_txnid().data(),
      table_cfg.metadata_txnid().size());

  return fetchMetadataFile(ns, table_id, txid, file);
}

Status MetadataClient::fetchMetadataFile(
    const String& ns,
    const String& table_id,
    const SHA1Hash& txnid,
    RefPtr<MetadataFile>* file) {
  return downloadMetadataFile(ns, table_id, txnid, file);
}

ReturnCode MetadataClient::downloadMetadataFile(
    const String& ns,
    const String& table_id,
    const SHA1Hash& txnid,
    RefPtr<MetadataFile>* file) {
  auto table_cfg = cdir_->getTableConfig(ns, table_id);
  auto idle_timeout = config_->getInt("server.s2s_idle_timeout", 0);
  auto io_timeout = config_->getInt("server.s2s_io_timeout", 0);

  native_transport::MetaGetfileFrame m_frame;
  m_frame.setDatabase(ns);
  m_frame.setTable(table_id);
  m_frame.setTransactionID(txnid);

  for (const auto& s : table_cfg.metadata_servers()) {
    auto server = cdir_->getServerConfig(s);
    if (server.server_status() != SERVER_UP) {
      logWarning("evqld", "metadata server is down: $0", s);
      continue;
    }

    native_transport::TCPClient client(
        conn_pool_,
        dns_cache_,
        io_timeout,
        idle_timeout);

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

    auto is = StringInputStream::fromString(ret_payload);
    file->reset(new MetadataFile());
    return (*file)->decode(is.get());
  }

  return Status(eIOError, "no metadata server responded");
}

Status MetadataClient::listPartitions(
    const String& ns,
    const String& table_id,
    const KeyRange& keyrange,
    PartitionListResponse* res) {
  auto table_cfg = cdir_->getTableConfig(ns, table_id);
  auto idle_timeout = config_->getInt("server.s2s_idle_timeout", 0);
  auto io_timeout = config_->getInt("server.s2s_io_timeout", 0);

  PartitionListRequest req;
  req.set_db_namespace(ns);
  req.set_table_id(table_id);
  req.set_keyrange_begin(keyrange.begin);
  req.set_keyrange_end(keyrange.end);

  Buffer req_payload;
  req_payload.append((char) 0);
  msg::encode(req, &req_payload);

  for (const auto& s : table_cfg.metadata_servers()) {
    auto server = cdir_->getServerConfig(s);
    if (server.server_status() != SERVER_UP) {
      logWarning("evqld", "metadata server is down: $0", s);
      continue;
    }

    native_transport::TCPClient client(
        conn_pool_,
        dns_cache_,
        io_timeout,
        idle_timeout);

    auto rc = client.connect(server.server_addr(), true);
    if (!rc.isSuccess()) {
      logWarning(
          "evqld",
          "can't connect to metadata server: $0",
          rc.getMessage());
      continue;
    }

    rc = client.sendFrame(
        EVQL_OP_META_LISTPARTITIONS,
        0,
        req_payload.data(),
        req_payload.size());

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
      case EVQL_OP_META_LISTPARTITIONS_RESULT:
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

    msg::decode<PartitionListResponse>(ret_payload, res);
    return Status::success();
  }

  return Status(eIOError, "no metadata server responded");
}

Status MetadataClient::findPartition(
    const String& ns,
    const String& table_id,
    const String& key,
    PartitionFindResponse* res) {
  auto table_cfg = cdir_->getTableConfig(ns, table_id);
  auto idle_timeout = config_->getInt("server.s2s_idle_timeout", 0);
  auto io_timeout = config_->getInt("server.s2s_io_timeout", 0);

  PartitionFindRequest req;
  req.set_db_namespace(ns);
  req.set_table_id(table_id);
  req.set_key(key);
  req.set_min_sequence_number(table_cfg.metadata_txnseq());
  req.set_keyspace(getKeyspace(table_cfg.config()));

  if (cache_->get(req, res)) {
    return Status::success();
  }

  Buffer req_payload;
  req_payload.append((char) 0);
  msg::encode(req, &req_payload);

  for (const auto& s : table_cfg.metadata_servers()) {
    auto server = cdir_->getServerConfig(s);
    if (server.server_status() != SERVER_UP) {
      logWarning("evqld", "metadata server is down: $0", s);
      continue;
    }

    native_transport::TCPClient client(
        conn_pool_,
        dns_cache_,
        io_timeout,
        idle_timeout);

    auto rc = client.connect(server.server_addr(), true);
    if (!rc.isSuccess()) {
      logWarning(
          "evqld",
          "can't connect to metadata server: $0",
          rc.getMessage());
      continue;
    }

    rc = client.sendFrame(
        EVQL_OP_META_FINDPARTITION,
        0,
        req_payload.data(),
        req_payload.size());

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
      case EVQL_OP_META_FINDPARTITION_RESULT:
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

    msg::decode<PartitionFindResponse>(ret_payload, res);
    cache_->store(req, *res);
    return Status::success();
  }

  return Status(eIOError, "no metadata server responded");
}

Status MetadataClient::findOrCreatePartition(
    const String& ns,
    const String& table_id,
    const String& key,
    PartitionFindResponse* res) {
  auto table_cfg = cdir_->getTableConfig(ns, table_id);
  auto idle_timeout = config_->getInt("server.s2s_idle_timeout", 0);
  auto io_timeout = config_->getInt("server.s2s_io_timeout", 0);

  PartitionFindRequest req;
  req.set_db_namespace(ns);
  req.set_table_id(table_id);
  req.set_key(key);
  req.set_allow_create(true);
  req.set_min_sequence_number(table_cfg.metadata_txnseq());
  req.set_keyspace(getKeyspace(table_cfg.config()));

  if (cache_->get(req, res)) {
    return Status::success();
  }

  Buffer req_payload;
  req_payload.append((char) 0);
  msg::encode(req, &req_payload);

  for (const auto& s : table_cfg.metadata_servers()) {
    auto server = cdir_->getServerConfig(s);
    if (server.server_status() != SERVER_UP) {
      logWarning("evqld", "metadata server is down: $0", s);
      continue;
    }

    native_transport::TCPClient client(
        conn_pool_,
        dns_cache_,
        io_timeout,
        idle_timeout);

    auto rc = client.connect(server.server_addr(), true);
    if (!rc.isSuccess()) {
      logWarning(
          "evqld",
          "can't connect to metadata server: $0",
          rc.getMessage());
      continue;
    }

    rc = client.sendFrame(
        EVQL_OP_META_FINDPARTITION,
        0,
        req_payload.data(),
        req_payload.size());

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
      case EVQL_OP_META_FINDPARTITION_RESULT:
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

    msg::decode<PartitionFindResponse>(ret_payload, res);
    cache_->store(req, *res);
    return Status::success();
  }

  return Status(eIOError, "no metadata server responded");
}

} // namespace eventql

