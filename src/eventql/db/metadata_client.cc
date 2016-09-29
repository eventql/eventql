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
#include <algorithm>
#include "eventql/db/metadata_client.h"
#include "eventql/db/metadata_store.h"
#include "eventql/db/partition_discovery.h"
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
    store_(store),
    conn_pool_(conn_pool),
    dns_cache_(dns_cache) {}

Status MetadataClient::fetchLatestMetadataFile(
    const String& ns,
    const String& table_id,
    RefPtr<MetadataFile>* file,
    bool allow_cache /* = true */) {
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
  /* check if a local copy of the file exists */
  if (store_->hasMetadataFile(ns, table_id, txnid)) {
    return store_->getMetadataFile(ns, table_id, txnid, file);
  }

  /* grab advisory download lock */
  auto locks = getAdvisoryLocks(ns, table_id);
  std::unique_lock<std::mutex> lock_holder(locks->download_lock);

  /* double-check if a local copy of the file exists */
  if (store_->hasMetadataFile(ns, table_id, txnid)) {
    return store_->getMetadataFile(ns, table_id, txnid, file);
  }

  /* download the file */
  auto download_rc = downloadMetadataFile(ns, table_id, txnid, file);
  if (!download_rc.isSuccess()) {
    return download_rc;
  }

  /* store the downloaded file locally */
  auto store_rc = store_->storeMetadataFile(ns, table_id, **file);
  if (!store_rc.isSuccess()) {
    logWarning("evqld", "metadata file store failed: $0", store_rc.message());
  }

  return ReturnCode::success();
}

ReturnCode MetadataClient::downloadMetadataFile(
    const String& ns,
    const String& table_id,
    const SHA1Hash& txnid,
    RefPtr<MetadataFile>* file) {
  logDebug(
      "evqld",
      "Downloading metadata file); db=$0 table=$1 txnid=$2",
      ns,
      table_id,
      txnid.toString());

  auto table_cfg = cdir_->getTableConfig(ns, table_id);
  auto idle_timeout = config_->getInt("server.s2s_idle_timeout", 0);
  auto io_timeout = config_->getInt("server.s2s_io_timeout", 0);

  native_transport::MetaGetfileFrame m_frame;
  m_frame.setDatabase(ns);
  m_frame.setTable(table_id);
  m_frame.setTransactionID(txnid);

  std::vector<std::string> servers(
      table_cfg.metadata_servers().begin(),
      table_cfg.metadata_servers().end());

  std::random_shuffle(servers.begin(), servers.end());

  for (const auto& s : servers) {
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

MetadataClientLocks* MetadataClient::getAdvisoryLocks(
    const String& ns,
    const String& table_id) {
  auto lock_key = StringUtil::format("$0~$1", ns, table_id);

  std::unique_lock<std::mutex> lockmap_mutex_holder(lockmap_mutex_);
  auto& lock_iter = lockmap_[lock_key];
  if (!lock_iter) {
    lock_iter.reset(new MetadataClientLocks());
  }

  return lock_iter.get();
}

Status MetadataClient::listPartitions(
    const String& ns,
    const String& table_id,
    const KeyRange& keyrange,
    PartitionListResponse* response) {
  RefPtr<MetadataFile> file;
  {
    auto rc = fetchLatestMetadataFile(
        ns,
        table_id,
        &file);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  auto iter = file->getPartitionMapRangeBegin(keyrange.begin);
  auto end = file->getPartitionMapRangeEnd(keyrange.end);
  for (; iter != end; ++iter) {
    auto e = response->add_partitions();
    e->set_partition_id(iter->partition_id.data(), iter->partition_id.size());
    e->set_keyrange_begin(iter->begin);
    if (iter + 1 != file->getPartitionMapEnd()) {
      e->set_keyrange_end(iter[1].begin);
    }

    for (const auto& s : iter->servers) {
      e->add_servers(s.server_id);
    }
    for (const auto& s : iter->servers_leaving) {
      e->add_servers(s.server_id);
    }
  }

  return Status::success();
}

Status MetadataClient::findPartition(
    const String& ns,
    const String& table_id,
    const String& key,
    bool allow_create,
    PartitionFindResponse* res) {
  auto table_cfg = cdir_->getTableConfig(ns, table_id);

  if (!table_cfg.config().enable_finite_partitions() &&
      !table_cfg.config().enable_user_defined_partitions()) {
    allow_create = false;
  }

  PartitionFindRequest req;
  req.set_db_namespace(ns);
  req.set_table_id(table_id);
  req.set_key(key);
  req.set_allow_create(allow_create);
  req.set_min_sequence_number(table_cfg.metadata_txnseq());
  req.set_keyspace(getKeyspace(table_cfg.config()));

  return findPartition(req, res);
}

Status MetadataClient::findPartition(
    const PartitionFindRequest& request,
    PartitionFindResponse* response) {
  if (cache_->get(request, response)) {
    return Status::success();
  }

  RefPtr<MetadataFile> file;
  {
    auto rc = fetchLatestMetadataFile(
        request.db_namespace(),
        request.table_id(),
        &file);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  auto partition = file->getPartitionMapAt(request.key());
  if (partition == file->getPartitionMapEnd()) {
    if (!request.allow_create()) {
      return Status(eRuntimeError, "partition not found");
    }

    return createPartition(request, response);
  }

  if (request.min_sequence_number() > file->getSequenceNumber()) {
      return Status(eRuntimeError, "metadata file is too old");
  }

  response->set_sequence_number(file->getSequenceNumber());
  response->set_partition_id(
      partition->partition_id.data(),
      partition->partition_id.size());

  response->set_partition_keyrange_begin(partition->begin);
  if (file->hasFinitePartitions()) {
    response->set_partition_keyrange_end(partition->end);
  } else if (file->hasUserDefinedPartitions()) {
    response->set_partition_keyrange_end(partition->begin);
  } else {
    auto next = partition + 1;
    if (next == file->getPartitionMapEnd()) {
      response->set_partition_keyrange_end("");
    } else {
      response->set_partition_keyrange_end(next->begin);
    }
  }

  for (const auto& s : partition->servers) {
    response->add_servers_for_insert(s.server_id);
  }
  for (const auto& s : partition->servers_leaving) {
    response->add_servers_for_insert(s.server_id);
  }

  cache_->store(request, *response);
  return Status::success();
}

Status MetadataClient::createPartition(
    const PartitionFindRequest& request,
    PartitionFindResponse* response) {
  auto locks = getAdvisoryLocks(
       request.db_namespace(),
       request.table_id());

  std::unique_lock<std::mutex> lock_holder(locks->create_lock);

  if (cache_->get(request, response)) {
    return Status::success();
  }

  logDebug(
      "evqld",
      "Creating new partition (remote); db=$0 table=$1",
      request.db_namespace(),
      request.table_id());

  auto idle_timeout = config_->getInt("server.s2s_idle_timeout", 0);
  auto io_timeout = config_->getInt("server.s2s_io_timeout", 0);
  auto table_cfg = cdir_->getTableConfig(
      request.db_namespace(),
      request.table_id());

  Buffer req_payload;
  req_payload.append((char) 0);
  msg::encode(request, &req_payload);

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

    msg::decode<PartitionFindResponse>(ret_payload, response);
    cache_->store(request, *response);
    return Status::success();
  }

  return Status(eIOError, "no metadata server responded");
}

Status MetadataClient::discoverPartition(
    PartitionDiscoveryRequest request,
    PartitionDiscoveryResponse* response) {
  request.set_requester_id(cdir_->getServerID());

  /* try discovery without lock */
  RefPtr<MetadataFile> file;
  {
    auto rc = fetchLatestMetadataFile(
        request.db_namespace(),
        request.table_id(),
        &file);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  if (file->getSequenceNumber() >= request.min_txnseq()) {
    PartitionDiscoveryResponse res;
    auto rc = PartitionDiscovery::discoverPartition(
        file.get(),
        request,
        &res);

    if (rc.isSuccess() && res.code() != PDISCOVERY_UNKNOWN) {
      *response = res;
      return rc;
    }
  }

  /* if this fails, grab a lock and try to retrieve the latest file */
  auto locks = getAdvisoryLocks(
       request.db_namespace(),
       request.table_id());

  std::unique_lock<std::mutex> lock_holder(locks->discover_lock);

  /* after we have the lock, try loading the latest file again */
  RefPtr<MetadataFile> latest_file;
  {
    auto rc = fetchLatestMetadataFile(
        request.db_namespace(),
        request.table_id(),
        &latest_file);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  /* if the id has changed, somebody else has probably gotten the latest file */
  if (latest_file->getSequenceNumber() > file->getSequenceNumber()) {
    return PartitionDiscovery::discoverPartition(
        latest_file.get(),
        request,
        response);
  }

  /* otherwise, we load the latest file, but this time disable local caching */
  {
    auto rc = fetchLatestMetadataFile(
        request.db_namespace(),
        request.table_id(),
        &latest_file,
        false);

    if (!rc.isSuccess()) {
      return rc;
    }
  }

  return PartitionDiscovery::discoverPartition(
      latest_file.get(),
      request,
      response);
}

} // namespace eventql

