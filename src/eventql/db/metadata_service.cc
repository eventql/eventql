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
#include "eventql/db/metadata_service.h"
#include "eventql/db/partition_discovery.h"
#include "eventql/db/metadata_coordinator.h"
#include "eventql/util/random.h"
#include "eventql/util/logging.h"
#include "eventql/db/server_allocator.h"

namespace eventql {

MetadataService::MetadataService(
    ConfigDirectory* cdir,
    MetadataStore* metadata_store) :
    cdir_(cdir),
    metadata_store_(metadata_store) {}

Status MetadataService::getMetadataFile(
    const String& ns,
    const String& table_name,
    const SHA1Hash& transaction_id,
    RefPtr<MetadataFile>* file) const {
  return metadata_store_->getMetadataFile(
      ns,
      table_name,
      transaction_id,
      file);
}

Status MetadataService::getMetadataFile(
    const String& ns,
    const String& table_name,
    RefPtr<MetadataFile>* file) const {
  auto table_cfg = cdir_->getTableConfig(ns, table_name);
  return metadata_store_->getMetadataFile(
      ns,
      table_name,
      SHA1Hash(
          table_cfg.metadata_txnid().data(),
          table_cfg.metadata_txnid().size()),
      file);
}

Status MetadataService::createMetadataFile(
    const String& ns,
    const String& table_name,
    const MetadataFile& file) const {
  if (file.getSequenceNumber() != 1) {
    return Status(eIllegalArgumentError, "sequence number must be 1");
  }

  return metadata_store_->storeMetadataFile(ns, table_name, file);
}

Status MetadataService::performMetadataOperation(
    const String& ns,
    const String& table_name,
    const MetadataOperation& op,
    MetadataOperationResult* result) {
  RefPtr<MetadataFile> input_file;
  {
    auto rc = metadata_store_->getMetadataFile(
        ns,
        table_name,
        op.getInputTransactionID(),
        &input_file);

    if (!rc.isSuccess()) {
      return rc;
    }
  }

  assert(input_file->getTransactionID() == op.getInputTransactionID());
  Vector<MetadataFile::PartitionMapEntry> new_pmap;
  {
    auto rc = op.perform(*input_file, &new_pmap);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  MetadataFile output_file(
      op.getOutputTransactionID(),
      input_file->getSequenceNumber() + 1,
      input_file->getKeyspaceType(),
      new_pmap,
      input_file->getFlags());

  SHA1Hash output_file_checksum;
  {
    auto rc = output_file.computeChecksum(&output_file_checksum);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  result->set_metadata_file_checksum(
      output_file_checksum.data(),
      output_file_checksum.size());

  return metadata_store_->storeMetadataFile(ns, table_name, output_file);
}

Status MetadataService::discoverPartition(
    const PartitionDiscoveryRequest& request,
    PartitionDiscoveryResponse* response) {
  RefPtr<MetadataFile> file;
  {
    auto rc = getMetadataFile(
        request.db_namespace(),
        request.table_id(),
        &file);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  if (file->getSequenceNumber() < request.min_txnseq()) {
    return Status(eRuntimeError, "metadata file is too old");
  }

  return PartitionDiscovery::discoverPartition(
      file.get(),
      request,
      response);
}

Status MetadataService::listPartitions(
    const PartitionListRequest& request,
    PartitionListResponse* response) {
  RefPtr<MetadataFile> file;
  {
    auto rc = getMetadataFile(
        request.db_namespace(),
        request.table_id(),
        &file);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  auto iter = file->getPartitionMapRangeBegin(request.keyrange_begin());
  auto end = file->getPartitionMapRangeEnd(request.keyrange_end());
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

Status MetadataService::findPartition(
    const PartitionFindRequest& request,
    PartitionFindResponse* response) {
  RefPtr<MetadataFile> file;
  {
    auto rc = getMetadataFile(
        request.db_namespace(),
        request.table_id(),
        &file);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  auto partition = file->getPartitionMapAt(request.key());
  if (partition == file->getPartitionMapEnd()) {
    if (request.allow_create()) {
      return createPartition(request, response);
    } else {
      return Status(eRuntimeError, "partition not found");
    }
  }

  response->set_partition_id(
      partition->partition_id.data(),
      partition->partition_id.size());

  for (const auto& s : partition->servers) {
    response->add_servers_for_insert(s.server_id);
  }
  for (const auto& s : partition->servers_leaving) {
    response->add_servers_for_insert(s.server_id);
  }

  return Status::success();
}

Status MetadataService::createPartition(
    const PartitionFindRequest& request,
    PartitionFindResponse* response) {
  auto table_config = cdir_->getTableConfig(
      request.db_namespace(),
      request.table_id());

  if (!table_config.config().enable_finite_partitions()) {
    return Status(eRuntimeError, "partition not found");
  }

  assert(
      table_config.config().partitioner() == TBL_PARTITION_TIMEWINDOW ||
      table_config.config().partitioner() == TBL_PARTITION_UINT64);

  uint64_t finite_partition_size = table_config.config().finite_partition_size();
  assert(finite_partition_size > 0);

  uint64_t p_point = std::stoull(
      decodePartitionKey(KEYSPACE_UINT64, request.key()));
  auto p_begin = (p_point / finite_partition_size) * finite_partition_size;
  auto p_end = p_begin + finite_partition_size;

  // FIXME: if p_begin or p_end overlap into another partition, adjust
  // accordingly

  auto partition_id = Random::singleton()->sha1();

  logDebug(
      "evqld",
      "Creating new finite partition; db=$0 table=$1 partition_id=$2 "
      "interval=[$3..$4)",
      request.db_namespace(),
      request.table_id(),
      partition_id.toString(),
      p_begin,
      p_end);

  std::vector<String> new_servers;
  {
    auto cconf = cdir_->getClusterConfig();
    ServerAllocator server_alloc(cdir_);
    auto rc = server_alloc.allocateServers(
        ServerAllocator::MUST_ALLOCATE,
        cconf.replication_factor(),
        Set<String>{},
        &new_servers);

    if (!rc.isSuccess()) {
      return rc;
    }
  }

  CreatePartitionOperation op;
  op.set_partition_id(partition_id.data(), partition_id.size());
  op.set_begin(
      encodePartitionKey(KEYSPACE_UINT64, StringUtil::toString(p_begin)));
  op.set_end(
      encodePartitionKey(KEYSPACE_UINT64, StringUtil::toString(p_end)));
  op.set_placement_id(Random::singleton()->random64());
  for (const auto& s : new_servers) {
    *op.add_servers() = s;
  }

  MetadataOperation envelope(
      request.db_namespace(),
      request.table_id(),
      METAOP_CREATE_PARTITION,
      SHA1Hash(
          table_config.metadata_txnid().data(),
          table_config.metadata_txnid().size()),
      Random::singleton()->sha1(),
      *msg::encode(op));

  MetadataCoordinator coordinator(cdir_);
  auto create_rc = coordinator.performAndCommitOperation(
      request.db_namespace(),
      request.table_id(),
      envelope);

  if (create_rc.isSuccess()) {
    response->set_partition_id(
        partition_id.data(),
        partition_id.size());

    for (const auto& s : new_servers) {
      response->add_servers_for_insert(s);
    }
  } else {
    RefPtr<MetadataFile> file;
    auto rc = getMetadataFile(
        request.db_namespace(),
        request.table_id(),
        &file);

    if (!rc.isSuccess()) {
      return rc;
    }

    auto partition = file->getPartitionMapAt(request.key());
    if (partition == file->getPartitionMapEnd()) {
      return Status(eRuntimeError, "partition create failed");
    }

    response->set_partition_id(
        partition->partition_id.data(),
        partition->partition_id.size());

    for (const auto& s : partition->servers) {
      response->add_servers_for_insert(s.server_id);
    }
    for (const auto& s : partition->servers_leaving) {
      response->add_servers_for_insert(s.server_id);
    }
  }

  return Status::success();
}

} // namespace eventql

