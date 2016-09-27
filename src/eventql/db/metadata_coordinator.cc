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
#include "eventql/db/metadata_coordinator.h"
#include "eventql/transport/native/client_tcp.h"
#include "eventql/transport/native/frames/error.h"
#include "eventql/transport/native/frames/meta_createfile.h"
#include "eventql/transport/native/frames/meta_performop.h"
#include <eventql/util/logging.h>

namespace eventql {

MetadataCoordinator::MetadataCoordinator(
    ConfigDirectory* cdir,
    ProcessConfig* config,
    native_transport::TCPConnectionPool* conn_pool,
    net::DNSCache* dns_cache) :
    cdir_(cdir),
    config_(config),
    conn_pool_(conn_pool),
    dns_cache_(dns_cache) {}

Status MetadataCoordinator::performAndCommitOperation(
    const String& ns,
    const String& table_name,
    MetadataOperation op,
    MetadataOperationResult* res /* = nullptr */) {
  auto table_config = cdir_->getTableConfig(ns, table_name);
  SHA1Hash input_txid(
      table_config.metadata_txnid().data(),
      table_config.metadata_txnid().size());

  if (!(input_txid == op.getInputTransactionID())) {
    return Status(eConcurrentModificationError, "concurrent modification");
  }

  Vector<String> servers;
  for (const auto& s : table_config.metadata_servers()) {
    servers.emplace_back(s);
  }

  auto rc = performOperation(ns, table_name, op, servers, res);
  if (!rc.isSuccess()) {
    return rc;
  }

  auto output_txid = op.getOutputTransactionID();
  table_config.set_metadata_txnid(output_txid.data(), output_txid.size());
  table_config.set_metadata_txnseq(table_config.metadata_txnseq() + 1);
  cdir_->updateTableConfig(table_config);
  return Status::success();
}

Status MetadataCoordinator::performOperation(
    const String& ns,
    const String& table_name,
    MetadataOperation op,
    const Vector<String>& servers,
    MetadataOperationResult* res /* = nullptr */) {
  size_t num_servers = servers.size();
  if (num_servers == 0) {
    return Status(eIllegalArgumentError, "server list can't be empty");
  }

  size_t failures = 0;
  Set<SHA1Hash> metadata_file_checksums;
  for (const auto& s : servers) {
    MetadataOperationResult result;
    auto rc = performOperation(ns, table_name, op, s, &result);
    if (rc.isSuccess()) {
      metadata_file_checksums.emplace(
          SHA1Hash(
              result.metadata_file_checksum().data(),
              result.metadata_file_checksum().size()));

      if (res) {
        *res = result;
      }
    } else {
      logDebug(
          "evqld",
          "error while performing metadata operation: $0",
          rc.message());
      ++failures;
    }
  }

  if (metadata_file_checksums.size() > 1) {
    return Status(eRuntimeError, "metadata operation would corrupt file");
  }

  size_t max_failures = 0;
  if (num_servers > 1) {
    max_failures = (num_servers - 1) / 2;
  }

  if (failures <= max_failures) {
    return Status::success();
  } else {
    return Status(eRuntimeError, "error while performing metadata operation");
  }
}

Status MetadataCoordinator::performOperation(
    const String& ns,
    const String& table_name,
    MetadataOperation op,
    const String& server,
    MetadataOperationResult* result) {
  auto server_cfg = cdir_->getServerConfig(server);
  if (server_cfg.server_addr().empty()) {
    return Status(eRuntimeError, "server is offline");
  }

  logDebug(
      "evqld",
      "Performing metadata operation on: $0/$1 ($2->$3) on $4 ($5)",
      ns,
      table_name,
      op.getInputTransactionID().toString(),
      op.getOutputTransactionID().toString(),
      server,
      server_cfg.server_addr());

  std::string req_body;
  {
    auto os = StringOutputStream::fromString(&req_body);
    auto rc = op.encode(os.get());
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  native_transport::MetaPerformopFrame m_frame;
  m_frame.setDatabase(ns);
  m_frame.setTable(table_name);
  m_frame.setOperation(req_body);

  auto idle_timeout = config_->getInt("server.s2s_idle_timeout", 0);
  auto io_timeout = config_->getInt("server.s2s_io_timeout", 0);
  native_transport::TCPClient client(
      conn_pool_,
      dns_cache_,
      io_timeout,
      idle_timeout);

  auto rc = client.connect(server_cfg.server_addr(), true);
  if (!rc.isSuccess()) {
    return rc;
  }

  rc = client.sendFrame(&m_frame, 0);
  if (!rc.isSuccess()) {
    return rc;
  }

  uint16_t ret_opcode = 0;
  uint16_t ret_flags;
  std::string ret_payload;
  rc = client.recvFrame(&ret_opcode, &ret_flags, &ret_payload, idle_timeout);
  if (!rc.isSuccess()) {
    return rc;
  }

  switch (ret_opcode) {
    case EVQL_OP_META_PERFORMOP_RESULT:
      break;
    case EVQL_OP_ERROR: {
      native_transport::ErrorFrame eframe;
      eframe.parseFrom(ret_payload.data(), ret_payload.size());
      return ReturnCode::error("ERUNTIME", eframe.getError());
    }
    default:
      return ReturnCode::errorf(
          "ERUNTIME",
          "invalid opcode for META_PERFORMOP: $0",
          ret_opcode);
  }

  *result = msg::decode<MetadataOperationResult>(ret_payload);
  return ReturnCode::success();
}

Status MetadataCoordinator::createFile(
    const String& ns,
    const String& table_name,
    const MetadataFile& file,
    const Vector<String>& servers) {
  size_t num_servers = servers.size();
  if (num_servers == 0) {
    return Status(eIllegalArgumentError, "server list can't be empty");
  }

  size_t failures = 0;
  for (const auto& s : servers) {
    auto rc = createFile(ns, table_name, file, s);
    if (!rc.isSuccess()) {
      logDebug("evqld", "error while creating metadata file: $0", rc.message());
      ++failures;
    }
  }

  size_t max_failures = 0;
  if (num_servers > 1) {
    max_failures = (num_servers - 1) / 2;
  }

  if (failures <= max_failures) {
    return Status::success();
  } else {
    return Status(eRuntimeError, "error while creating metadata file");
  }
}

Status MetadataCoordinator::createFile(
    const String& ns,
    const String& table_name,
    const MetadataFile& file,
    const String& server_id) {
  auto server = cdir_->getServerConfig(server_id);
  if (server.server_addr().empty()) {
    return Status(eRuntimeError, "server is offline");
  }

  logDebug(
      "evqld",
      "Creating metadata file: $0/$1/$2 on $3 ($4)",
      ns,
      table_name,
      file.getTransactionID(),
      server_id,
      server.server_addr());

  std::string req_body;
  {
    auto os = StringOutputStream::fromString(&req_body);
    auto rc = file.encode(os.get());
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  native_transport::MetaCreatefileFrame m_frame;
  m_frame.setDatabase(ns);
  m_frame.setTable(table_name);
  m_frame.setBody(req_body);

  auto idle_timeout = config_->getInt("server.s2s_idle_timeout", 0);
  auto io_timeout = config_->getInt("server.s2s_io_timeout", 0);
  native_transport::TCPClient client(
      conn_pool_,
      dns_cache_,
      io_timeout,
      idle_timeout);

  auto rc = client.connect(server.server_addr(), true);
  if (!rc.isSuccess()) {
    return rc;
  }

  rc = client.sendFrame(&m_frame, 0);
  if (!rc.isSuccess()) {
    return rc;
  }

  uint16_t ret_opcode = 0;
  uint16_t ret_flags;
  std::string ret_payload;
  rc = client.recvFrame(&ret_opcode, &ret_flags, &ret_payload, idle_timeout);
  if (!rc.isSuccess()) {
    return rc;
  }

  switch (ret_opcode) {
    case EVQL_OP_ACK:
      break;
    case EVQL_OP_ERROR: {
      native_transport::ErrorFrame eframe;
      eframe.parseFrom(ret_payload.data(), ret_payload.size());
      return ReturnCode::error("ERUNTIME", eframe.getError());
    }
    default:
      return ReturnCode::errorf(
          "ERUNTIME",
          "invalid opcode for META_CREATEFILE: $0",
          ret_opcode);
  }

  return ReturnCode::success();
}

} // namespace eventql

