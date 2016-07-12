/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <eventql/db/LSMPartitionReplication.h>
#include <eventql/db/LSMPartitionReader.h>
#include <eventql/db/LSMPartitionWriter.h>
#include <eventql/db/ReplicationScheme.h>
#include <eventql/db/ReplicationWorker.h>
#include <eventql/util/logging.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/util/protobuf/MessageEncoder.h>
#include <eventql/util/protobuf/MessagePrinter.h>
#include <eventql/io/cstable/RecordMaterializer.h>
#include <eventql/db/metadata_operations.pb.h>
#include <eventql/db/metadata_coordinator.h>

#include "eventql/eventql.h"

namespace eventql {

const size_t LSMPartitionReplication::kMaxBatchSizeRows = 1024;
const size_t LSMPartitionReplication::kMaxBatchSizeBytes = 1024 * 1024 * 2; // 2 MB

LSMPartitionReplication::LSMPartitionReplication(
    RefPtr<Partition> partition,
    RefPtr<ReplicationScheme> repl_scheme,
    ConfigDirectory* cdir,
    http::HTTPConnectionPool* http) :
    PartitionReplication(partition, repl_scheme, http), cdir_(cdir) {}

bool LSMPartitionReplication::needsReplication() const {
  // check if we have seen the latest metadata transaction, otherwise enqueue
  if (snap_->state.last_metadata_txnid().empty()) {
    return true;
  }

  SHA1Hash last_seen_txid(
      snap_->state.last_metadata_txnid().data(),
      snap_->state.last_metadata_txnid().size());

  auto last_txid = partition_->getTable()->getLastMetadataTransaction();
  if (last_txid.getTransactionID() != last_seen_txid) {
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
  auto& writer = dynamic_cast<LSMPartitionWriter&>(*partition_->getWriter());
  auto repl_state = writer.fetchReplicationState();
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
  auto server_cfg = cdir_->getServerConfig(replica.server_id());
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
          replica.keyrange_begin(),
          replica.keyrange_end(),
          &upload_builder,
          &upload_skiplist,
          &upload_nskipped);

      // FIXME: send metadata list to remote and merge skiplist

      // skip batch if no records to upload
      if (upload_nskipped == upload_batchsize) {
        continue;
      }

      // read data columns
      readBatchPayload(
          cstable.get(),
          upload_batchsize,
          upload_skiplist,
          &upload_builder);

      // upload batch
      bytes_sent += uploadBatchTo(
          server_cfg.server_addr(),
          SHA1Hash(replica.partition_id().data(), replica.partition_id().size()),
          upload_builder.get());

      records_sent += upload_batchsize - upload_nskipped;
      replication_info->setTargetHostStatus(bytes_sent, records_sent);
    }
  }
}

bool LSMPartitionReplication::replicate(ReplicationInfo* replication_info) {
  logTrace(
      "z1.replication",
      "Replicating partition $0/$1/$2",
      snap_->state.tsdb_namespace(),
      snap_->state.table_key(),
      snap_->key.toString());

  // if there is a new metadata transaction, fetch and apply it
  auto rc = fetchAndApplyMetadataTransaction();
  if (!rc.isSuccess()) {
    RAISEF(
        kRuntimeError,
        "error while applying metadata transaction: $0",
        rc.message());
  }

  // push all outstanding data to all replication targets
  auto& writer = dynamic_cast<LSMPartitionWriter&>(*partition_->getWriter());
  auto repl_state = writer.fetchReplicationState();
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
        "z1.replication",
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
        "z1.replication",
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

  MetadataCoordinator coordinator(cdir_);
  PartitionDiscoveryResponse discovery_response;
  auto rc = coordinator.discoverPartition(
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
      "z1.replication",
      "Finalizing split for partition $0/$1/$2",
      snap_->state.tsdb_namespace(),
      snap_->state.table_key(),
      snap_->key.toString());

  FinalizeSplitOperation op;
  op.set_partition_id(snap_->key.data(), snap_->key.size());

  auto table_config = cdir_->getTableConfig(
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

  MetadataCoordinator coordinator(cdir_);
  return coordinator.performAndCommitOperation(
      snap_->state.tsdb_namespace(),
      snap_->state.table_key(),
      envelope);
}

Status LSMPartitionReplication::finalizeJoin(const ReplicationTarget& target) {
  logDebug(
      "z1.replication",
      "Finalizing join for partition $0/$1/$2, server $3",
      snap_->state.tsdb_namespace(),
      snap_->state.table_key(),
      snap_->key.toString(),
      target.server_id());

  FinalizeJoinOperation op;
  op.set_partition_id(target.partition_id());
  op.set_server_id(target.server_id());
  op.set_placement_id(target.placement_id());

  auto table_config = cdir_->getTableConfig(
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

  MetadataCoordinator coordinator(cdir_);
  return coordinator.performAndCommitOperation(
      snap_->state.tsdb_namespace(),
      snap_->state.table_key(),
      envelope);
}

bool LSMPartitionReplication::shouldDropPartition() const {
  if (snap_->state.lifecycle_state() == PDISCOVERY_UNLOAD &&
      !needsReplication()) {
    return true;
  } else {
    return false;
  }
}

void LSMPartitionReplication::readBatchMetadata(
    cstable::CSTableReader* cstable,
    size_t upload_batchsize,
    size_t start_sequence,
    size_t start_position,
    bool has_skiplist,
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
  auto pkey_fieldname = partition_->getTable()->getPartitionKey();
  auto keyspace = partition_->getTable()->getKeyspaceType();
  auto id_col = cstable->getColumnReader("__lsm_id");
  auto version_col = cstable->getColumnReader("__lsm_version");
  auto pkey_col = cstable->getColumnReader(pkey_fieldname);
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

  pkey_col->rewind();
  for (size_t i = 0; i < start_position; ++i) {
    pkey_col->skipValue();
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

size_t LSMPartitionReplication::uploadBatchTo(
    const String& host,
    const SHA1Hash& target_partition_id,
    const ShreddedRecordList& batch) {
  Buffer body;
  auto body_os = BufferOutputStream::fromBuffer(&body);
  batch.encode(body_os.get());

  URI uri(
      StringUtil::format(
          "http://$0/tsdb/replicate?namespace=$1&table=$2&partition=$3",
          host,
          URI::urlEncode(snap_->state.tsdb_namespace()),
          URI::urlEncode(snap_->state.table_key()),
          target_partition_id.toString()));

  http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  req.addBody(body.data(), body.size());

  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 201) {
    RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
  }

  return body.size();
}

} // namespace tdsb

