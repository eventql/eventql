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
#include <eventql/db/partition_replication.h>
#include <eventql/db/partition_writer.h>
#include <eventql/db/metadata_operations.pb.h>
#include <eventql/db/metadata_coordinator.h>
#include <eventql/db/metadata_client.h>
#include <eventql/db/replication_state.h>
#include <eventql/db/replication_worker.h>
#include <eventql/config/config_directory.h>
#include <eventql/util/logging.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/protobuf/msg.h>
#include "eventql/eventql.h"
#include "eventql/transport/native/client_tcp.h"
#include "eventql/transport/native/frames/error.h"
#include "eventql/transport/native/frames/repl_insert.h"

namespace eventql {

const char PartitionReplication::kStateFileName[] = "_repl";

PartitionReplication::PartitionReplication(
    RefPtr<Partition> partition) :
    partition_(partition),
    snap_(partition_->getSnapshot()) {}

ReplicationState PartitionReplication::fetchReplicationState(
    RefPtr<PartitionSnapshot> snap) {
  auto filepath = FileUtil::joinPaths(snap->base_path, kStateFileName);
  String tbl_uuid((char*) snap->uuid().data(), snap->uuid().size());

  if (FileUtil::exists(filepath)) {
    auto disk_state = msg::decode<ReplicationState>(FileUtil::read(filepath));

    if (disk_state.uuid() == tbl_uuid) {
      return disk_state;
    }
  }

  ReplicationState state;
  state.set_uuid(tbl_uuid);
  return state;
}

ReplicationState PartitionReplication::fetchReplicationState() const {
  return fetchReplicationState(snap_);
}

void PartitionReplication::commitReplicationState(
    const ReplicationState& state) {
  auto filepath = FileUtil::joinPaths(snap_->base_path, kStateFileName);
  auto buf = msg::encode(state);

  {
    auto file = File::openFile(
        filepath + "~",
        File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

    file.write(buf->data(), buf->size());
  }

  FileUtil::mv(filepath + "~", filepath);
}

const size_t LSMPartitionReplication::kMaxBatchSizeRows = 1024;
const size_t LSMPartitionReplication::kMaxBatchSizeBytes = 1024 * 1024 * 2; // 2 MB

LSMPartitionReplication::LSMPartitionReplication(
    RefPtr<Partition> partition,
    DatabaseContext* dbctx) :
    PartitionReplication(partition),
    dbctx_(dbctx) {}

bool LSMPartitionReplication::needsReplication() const {
  auto table_config = dbctx_->config_directory->getTableConfig(
      snap_->state.tsdb_namespace(),
      snap_->state.table_key());

  auto& writer = dynamic_cast<LSMPartitionWriter&>(*partition_->getWriter());
  auto repl_state = writer.fetchReplicationState();

  // check if we are dropped
  if (snap_->state.table_generation() < table_config.generation()) {
    return !repl_state.dropped();
  }

  // check if we have seen the latest metadata transaction, otherwise enqueue
  if (snap_->state.last_metadata_txnid().empty()) {
    return true;
  }

  SHA1Hash last_seen_txid(
      snap_->state.last_metadata_txnid().data(),
      snap_->state.last_metadata_txnid().size());

  SHA1Hash latest_txid(
      table_config.metadata_txnid().data(),
      table_config.metadata_txnid().size());

  if (latest_txid != last_seen_txid) {
    return true;
  }

  // check if we are splitting
  if (snap_->state.is_splitting()) {
    return true;
  }

  // check if we have joining servers
  if (snap_->state.has_joining_servers()) {
    return true;
  }

  // check if all replicas named in the current metadata transaction have seen
  // the latest sequence, otherwise enqueue
  auto head_offset = snap_->state.lsm_sequence();
  for (const auto& r : snap_->state.replication_targets()) {
    auto replica_offset = replicatedOffsetFor(repl_state, r);
    if (replica_offset < head_offset) {
      return true;
    }
  }

  return false;
}

void LSMPartitionReplication::replicateTo(
    const ReplicationTarget& replica,
    uint64_t replicated_offset,
    ReplicationInfo* replication_info) {
  auto server_cfg = dbctx_->config_directory->getServerConfig(
      replica.server_id());

  if (server_cfg.server_status() != SERVER_UP) {
    RAISE(kRuntimeError, "server is down");
  }

  SHA1Hash target_partition_id(
      replica.partition_id().data(),
      replica.partition_id().size());

  size_t records_sent = 0;
  size_t bytes_sent = 0;

  const auto& tables = snap_->state.lsm_tables();
  for (const auto& tbl : tables) {
    if (tbl.last_sequence() < replicated_offset) {
      continue;
    }

    // open cstable
    auto cstable_file = FileUtil::joinPaths(
        snap_->base_path,
        tbl.filename() + ".cst");
    auto cstable = cstable::CSTableReader::openFile(cstable_file);
    uint64_t nrecs = cstable->numRecords();
    auto pkey_col = cstable->getColumnReader(
        partition_->getTable()->getPartitionKey(),
        cstable::ColumnReader::Visibility::PRIVATE);

    // in batches
    for (uint64_t nrecs_cur = 0; nrecs_cur < nrecs; ) {
      auto upload_batchsize = std::min(nrecs - nrecs_cur, uint64_t(256)); // FIXME
      nrecs_cur += upload_batchsize;
      Vector<bool> upload_skiplist(upload_batchsize, true);
      size_t upload_nskipped = 0;
      ShreddedRecordListBuilder upload_builder;

      // read metadata
      readBatchMetadata(
          cstable.get(),
          upload_batchsize,
          replicated_offset,
          nrecs_cur,
          tbl.has_skiplist(),
          pkey_col.get(),
          replica.keyrange_begin(),
          replica.keyrange_end(),
          &upload_builder,
          &upload_skiplist,
          &upload_nskipped);

      // FIXME: send metadata list to remote and merge skiplist

      // FIXME: skip data columns if no records to upload
      // read data columns
      readBatchPayload(
          cstable.get(),
          upload_batchsize,
          upload_skiplist,
          &upload_builder);

      // skip batch if no records to upload
      if (upload_nskipped == upload_batchsize) {
        continue;
      }

      // upload batch
      auto rc = uploadBatchTo(
          server_cfg.server_addr(),
          SHA1Hash(replica.partition_id().data(), replica.partition_id().size()),
          upload_builder.get());

      if (!rc.isSuccess()) {
        RAISE(kRuntimeError, rc.getMessage());
      }

      records_sent += upload_batchsize - upload_nskipped;
      replication_info->setTargetHostStatus(bytes_sent, records_sent);
    }
  }
}

bool LSMPartitionReplication::replicate(ReplicationInfo* replication_info) {
  logTrace(
      "evqld",
      "Replicating partition $0/$1/$2",
      snap_->state.tsdb_namespace(),
      snap_->state.table_key(),
      snap_->key.toString());

  auto table_config = dbctx_->config_directory->getTableConfig(
      snap_->state.tsdb_namespace(),
      snap_->state.table_key());

  auto& writer = dynamic_cast<LSMPartitionWriter&>(*partition_->getWriter());
  auto repl_state = writer.fetchReplicationState();

  // if the partition generation is smaller than the table generation, this
  // means that the table was dropped and we should not replicate the partition
  // anymore
  if (snap_->state.table_generation() < table_config.generation()) {
    dbctx_->partition_map->dropPartition(
        snap_->state.tsdb_namespace(),
        snap_->state.table_key(),
        snap_->key);

    repl_state.set_dropped(true);
    writer.commitReplicationState(repl_state);

    return true;
  }

  // if there is a new metadata transaction, fetch and apply it
  auto rc = fetchAndApplyMetadataTransaction();
  if (!rc.isSuccess()) {
    RAISEF(
        kRuntimeError,
        "error while applying metadata transaction: $0",
        rc.message());
  }

  // push all outstanding data to all replication targets
  auto head_offset = snap_->state.lsm_sequence();
  bool dirty = false;
  bool success = true;
  HashMap<SHA1Hash, size_t> replicas_per_partition;
  Vector<ReplicationTarget> completed_joins;

  for (const auto& r : snap_->state.replication_targets()) {
    auto replica_offset = replicatedOffsetFor(repl_state, r);
    SHA1Hash target_partition_id(
        r.partition_id().data(),
        r.partition_id().size());

    if (replica_offset >= head_offset) {
      ++replicas_per_partition[target_partition_id];
      if (r.is_joining()) {
        completed_joins.emplace_back(r);
      }
      continue;
    }

    logDebug(
        "evqld",
        "Replicating partition $0/$1/$2 to $3 (replicated_seq: $4, head_seq: $5, $6 records)",
        snap_->state.tsdb_namespace(),
        snap_->state.table_key(),
        snap_->key.toString(),
        r.server_id(),
        replica_offset,
        head_offset,
        head_offset - replica_offset);

    replication_info->setTargetHost(r.server_id());

    try {
      replicateTo(r, replica_offset, replication_info);

      setReplicatedOffsetFor(&repl_state, r, head_offset);
      {
        auto& writer = dynamic_cast<LSMPartitionWriter&>(*partition_->getWriter());
          writer.commitReplicationState(repl_state);
      }

      dirty = true;
      ++replicas_per_partition[target_partition_id];

      if (r.is_joining()) {
        completed_joins.emplace_back(r);
      }
    } catch (const std::exception& e) {
      success = false;

      logError(
        "evqld",
        e,
        "Error while replicating partition $0/$1/$2 to $3",
        snap_->state.tsdb_namespace(),
        snap_->state.table_key(),
        snap_->key.toString(),
        r.server_id());
    }
  }

  // commit new replication state
  if (dirty) {
    auto& writer = dynamic_cast<LSMPartitionWriter&>(*partition_->getWriter());
    writer.commitReplicationState(repl_state);
  }

  // finalize joins
  for (const auto& t : completed_joins) {
    auto rc = finalizeJoin(t);
    if (!rc.isSuccess()) {
      logWarning("evqld", "error while finalizing join: $0", rc.message());
    }
  }

  // finalize split
  if (snap_->state.is_splitting() &&
      snap_->state.lifecycle_state() == PDISCOVERY_SERVE) {
    HashMap<SHA1Hash, size_t> servers_per_partition;
    for (const auto& r : snap_->state.replication_targets()) {
      SHA1Hash target_partition_id(
          r.partition_id().data(),
          r.partition_id().size());

      ++servers_per_partition[target_partition_id];
    }

    bool split_complete = snap_->state.split_partition_ids().size() > 0;
    for (const auto& p : snap_->state.split_partition_ids()) {
      SHA1Hash split_partition_id(p.data(), p.size());

      size_t total_servers = servers_per_partition[split_partition_id];
      size_t complete_servers = replicas_per_partition[split_partition_id];
      if (complete_servers > total_servers) {
        RAISE(kIllegalStateError);
      }

      size_t num_failures = total_servers - complete_servers;
      size_t max_failures = 0;
      if (total_servers > 1) {
        max_failures = (total_servers - 1) / 2;
      }

      if (total_servers < 1 || num_failures > max_failures) {
        split_complete = false;
      }
    }

    if (split_complete) {
      auto rc = finalizeSplit();
      if (!rc.isSuccess()) {
        logWarning("evqld", "error while finalizing split: $0", rc.message());
        return false;
      }
    }
  }

  return success;
}

Status LSMPartitionReplication::fetchAndApplyMetadataTransaction() {
  auto last_txid = partition_->getTable()->getLastMetadataTransaction();
  bool has_new_metadata_txn =
      snap_->state.last_metadata_txnid().empty() ||
      SHA1Hash(
          snap_->state.last_metadata_txnid().data(),
          snap_->state.last_metadata_txnid().size()) != last_txid.getTransactionID();

  if (has_new_metadata_txn) {
    auto rc = fetchAndApplyMetadataTransaction(last_txid);
    if (!rc.isSuccess()) {
      return rc;
    }

    snap_ = partition_->getSnapshot();
  }

  return Status::success();
}

Status LSMPartitionReplication::fetchAndApplyMetadataTransaction(
    MetadataTransaction txn) {
  PartitionDiscoveryRequest discovery_request;
  discovery_request.set_db_namespace(snap_->state.tsdb_namespace());
  discovery_request.set_table_id(snap_->state.table_key());
  discovery_request.set_min_txnseq(txn.getSequenceNumber());
  discovery_request.set_partition_id(snap_->state.partition_key());
  discovery_request.set_keyrange_begin(snap_->state.partition_keyrange_begin());
  discovery_request.set_keyrange_end(snap_->state.partition_keyrange_end());

  PartitionDiscoveryResponse discovery_response;
  auto rc = dbctx_->metadata_client->discoverPartition(
      discovery_request,
      &discovery_response);

  if (!rc.isSuccess()) {
    return rc;
  }

  auto writer = partition_->getWriter().asInstanceOf<LSMPartitionWriter>();
  return writer->applyMetadataChange(discovery_response);
}

Status LSMPartitionReplication::finalizeSplit() {
  logDebug(
      "evqld",
      "Finalizing split for partition $0/$1/$2",
      snap_->state.tsdb_namespace(),
      snap_->state.table_key(),
      snap_->key.toString());

  FinalizeSplitOperation op;
  op.set_partition_id(snap_->key.data(), snap_->key.size());

  auto table_config = dbctx_->config_directory->getTableConfig(
      snap_->state.tsdb_namespace(),
      snap_->state.table_key());
  MetadataOperation envelope(
      snap_->state.tsdb_namespace(),
      snap_->state.table_key(),
      METAOP_FINALIZE_SPLIT,
      SHA1Hash(
          table_config.metadata_txnid().data(),
          table_config.metadata_txnid().size()),
      Random::singleton()->sha1(),
      *msg::encode(op));

  return dbctx_->metadata_coordinator->performAndCommitOperation(
      snap_->state.tsdb_namespace(),
      snap_->state.table_key(),
      envelope);
}

Status LSMPartitionReplication::finalizeJoin(const ReplicationTarget& target) {
  logDebug(
      "evqld",
      "Finalizing join for partition $0/$1/$2, server $3",
      snap_->state.tsdb_namespace(),
      snap_->state.table_key(),
      snap_->key.toString(),
      target.server_id());

  FinalizeJoinOperation op;
  op.set_partition_id(target.partition_id());
  op.set_server_id(target.server_id());
  op.set_placement_id(target.placement_id());

  auto table_config = dbctx_->config_directory->getTableConfig(
      snap_->state.tsdb_namespace(),
      snap_->state.table_key());
  MetadataOperation envelope(
      snap_->state.tsdb_namespace(),
      snap_->state.table_key(),
      METAOP_FINALIZE_JOIN,
      SHA1Hash(
          table_config.metadata_txnid().data(),
          table_config.metadata_txnid().size()),
      Random::singleton()->sha1(),
      *msg::encode(op));

  return dbctx_->metadata_coordinator->performAndCommitOperation(
      snap_->state.tsdb_namespace(),
      snap_->state.table_key(),
      envelope);
}

bool LSMPartitionReplication::shouldDropPartition() const {
  if (snap_->state.lifecycle_state() != PDISCOVERY_UNLOAD) {
    return false;
  }

  if (snap_->head_arena.get() && snap_->head_arena->size() > 0) {
    return false;
  }

  if (snap_->compacting_arena.get() && snap_->compacting_arena->size() > 0) {
    return false;
  }

  if (needsReplication()) {
    return false;
  }

  if (snap_->state.replication_targets().size() == 0) {
    logWarning("evqld", "partition has no replication targets");
    return false;
  }

  return true;
}

void LSMPartitionReplication::readBatchMetadata(
    cstable::CSTableReader* cstable,
    size_t upload_batchsize,
    size_t start_sequence,
    size_t start_position,
    bool has_skiplist,
    cstable::ColumnReader* pkey_col,
    const String& keyrange_begin,
    const String& keyrange_end,
    ShreddedRecordListBuilder* upload_builder,
    Vector<bool>* upload_skiplist,
    size_t* upload_nskipped) {
  // read skip col
  if (has_skiplist) {
    auto skip_col = cstable->getColumnReader("__lsm_skip");
    for (size_t i = 0; i < upload_batchsize; ++i) {
      uint64_t rlvl;
      uint64_t dlvl;

      bool skip = false;
      if (skip_col.get()) {
        skip_col->readBoolean(&rlvl, &dlvl, &skip);
      }

      (*upload_skiplist)[i] = !skip;
    }
  }

  // read sequence_col
  auto sequence_col = cstable->getColumnReader("__lsm_sequence");
  for (size_t i = 0; i < upload_batchsize; ++i) {
    uint64_t rlvl;
    uint64_t dlvl;

    uint64_t sequence;
    sequence_col->readUnsignedInt(&rlvl, &dlvl, &sequence);

    if (sequence < start_sequence) {
      (*upload_skiplist)[i] = false;
    }
  }

  // read primary keys
  auto keyspace = partition_->getTable()->getKeyspaceType();
  auto id_col = cstable->getColumnReader("__lsm_id");
  auto version_col = cstable->getColumnReader("__lsm_version");
  for (size_t i = 0; i < upload_batchsize; ++i) {
    if ((*upload_skiplist)[i]) {
      uint64_t rlvl;
      uint64_t dlvl;

      String pkey_value;
      switch (keyspace) {
        case KEYSPACE_STRING: {
          pkey_col->readString(&rlvl, &dlvl, &pkey_value);
          break;
        }
        case KEYSPACE_UINT64: {
          uint64_t pkey_value_uint = 0;
          pkey_col->readUnsignedInt(&rlvl, &dlvl, &pkey_value_uint);
          pkey_value = String(
              (const char*) &pkey_value_uint,
              sizeof(uint64_t));
          break;
        }
      }

      if (!keyrange_begin.empty()) {
        if (comparePartitionKeys(keyspace, pkey_value, keyrange_begin) < 0) {
          (*upload_skiplist)[i] = false;
        }
      }

      if (!keyrange_end.empty()) {
        if (comparePartitionKeys(keyspace, pkey_value, keyrange_end) >= 0) {
          (*upload_skiplist)[i] = false;
        }
      }
    } else {
      pkey_col->skipValue();
    }
  }

  // read id & version
  for (size_t i = 0; i < upload_batchsize; ++i) {
    if ((*upload_skiplist)[i]) {
      uint64_t rlvl;
      uint64_t dlvl;

      String id_str;
      id_col->readString(&rlvl, &dlvl, &id_str);

      uint64_t version;
      version_col->readUnsignedInt(&rlvl, &dlvl, &version);

      upload_builder->addRecord(
          SHA1Hash(id_str.data(), id_str.size()),
          version);
    } else {
      id_col->skipValue();
      version_col->skipValue();
      ++(*upload_nskipped);
    }
  }
}

void LSMPartitionReplication::readBatchPayload(
    cstable::CSTableReader* cstable,
    size_t upload_batchsize,
    const Vector<bool>& upload_skiplist,
    ShreddedRecordListBuilder* upload_builder) {
  auto cstable_schema = cstable::TableSchema::fromProtobuf(
      *partition_->getTable()->schema());

  auto columns = cstable->columns();
  for (const auto& col : columns) {
    if (StringUtil::beginsWith(col.column_name, "__lsm")) {
      continue;
    }

    auto col_reader = cstable->getColumnReader(col.column_name);
    auto col_writer = upload_builder->getColumn(col.column_name);

    for (size_t i = 0; i < upload_batchsize; ) {
      uint64_t rlvl;
      uint64_t dlvl;

      String val;
      col_reader->readString(&rlvl, &dlvl, &val);

      if (upload_skiplist[i]) {
        if (dlvl == col.dlevel_max) {
          col_writer->addValue(rlvl, dlvl, val);
        } else {
          col_writer->addNull(rlvl, dlvl);
        }
      }

      if (col_reader->nextRepetitionLevel() == 0) {
        ++i;
      }
    }
  }
}

ReturnCode LSMPartitionReplication::uploadBatchTo(
    const String& host,
    const SHA1Hash& target_partition_id,
    const ShreddedRecordList& batch) {
  auto io_timeout = dbctx_->config->getInt("server.s2s_io_timeout", 0);
  auto idle_timeout = dbctx_->config->getInt("server.s2s_idle_timeout", 0);

  /* connect to host */
  native_transport::TCPClient client(
      dbctx_->connection_pool,
      dbctx_->dns_cache,
      io_timeout,
      idle_timeout);

  auto rc = client.connect(host, true);
  if (!rc.isSuccess()) {
    return rc;
  }

  /* build rpc payload */
  std::string rpc_body;
  auto rpc_body_os = StringOutputStream::fromString(&rpc_body);
  batch.encode(rpc_body_os.get());

  native_transport::ReplInsertFrame i_frame;
  i_frame.setDatabase(snap_->state.tsdb_namespace());
  i_frame.setTable(snap_->state.table_key());
  i_frame.setPartitionID(target_partition_id.toString());
  i_frame.setBody(rpc_body);

  /* send insert frame */
  rc = client.sendFrame(&i_frame, 0);
  if (!rc.isSuccess()) {
    return rc;
  }

  /* receive result */
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
      return ReturnCode::error("ERUNTIME", "invalid opcode");
  }

  return ReturnCode::success();
}

} // namespace tdsb

