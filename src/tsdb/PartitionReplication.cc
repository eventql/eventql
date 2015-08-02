/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/PartitionReplication.h>
#include <tsdb/ReplicationScheme.h>
#include <tsdb/RecordEnvelope.pb.h>
#include <stx/logging.h>
#include <stx/io/fileutil.h>
#include <stx/protobuf/msg.h>

using namespace stx;

namespace tsdb {

const char PartitionReplication::kStateFileName[] = "_repl";

PartitionReplication::PartitionReplication(
    RefPtr<Partition> partition,
    RefPtr<ReplicationScheme> repl_scheme,
    http::HTTPConnectionPool* http) :
    partition_(partition),
    snap_(partition_->getSnapshot()),
    repl_scheme_(repl_scheme),
    http_(http) {}

bool PartitionReplication::needsReplication() const {
  auto replicas = repl_scheme_->replicasFor(snap_->key);
  if (replicas.size() == 0) {
    return false;
  }

  auto repl_state = fetchReplicationState();
  auto head_offset = snap_->nrecs;
  for (const auto& r : replicas) {
    const auto& replica_offset = replicatedOffsetFor(repl_state, r.unique_id);
    if (replica_offset < head_offset) {
      return true;
    }
  }

  return false;
}

void PartitionReplication::replicateTo(
    const ReplicaRef& replica,
    uint64_t replicated_offset) {
  auto head_offset = snap_->nrecs;
  PartitionReader reader(snap_);

  while (replicated_offset < head_offset) {
    RecordEnvelopeList batch;

    reader.fetchRecords(replicated_offset, 1024, [this, &batch] (
        const SHA1Hash& record_id,
        const void* record_data,
        size_t record_size) {
      auto rec = batch.add_records();
      rec->set_tsdb_namespace(snap_->state.tsdb_namespace());
      rec->set_stream_key(snap_->state.table_key());
      rec->set_partition_key(snap_->key.toString());
      rec->set_record_id(record_id.toString());
      rec->set_record_data(record_data, record_size);
    });

    auto body = msg::encode(batch);
    URI uri(StringUtil::format("http://$0/tsdb/insert", replica.addr));
    http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
    req.addHeader("Host", uri.hostAndPort());
    req.addHeader("Content-Type", "application/fnord-msg");
    req.addBody(body->data(), body->size());

    auto res = http_->executeRequest(req);
    res.wait();

    const auto& r = res.get();
    if (r.statusCode() != 201) {
      RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
    }

    replicated_offset += batch.records().size();
  }
}

bool PartitionReplication::replicate() {
  auto replicas = repl_scheme_->replicasFor(snap_->key);
  if (replicas.size() == 0) {
    return true;
  }

  auto repl_state = fetchReplicationState();
  auto head_offset = snap_->nrecs;
  bool dirty = false;
  bool success = true;

  for (const auto& r : replicas) {
    const auto& replica_offset = replicatedOffsetFor(repl_state, r.unique_id);

    if (replica_offset < head_offset) {
      logDebug(
          "tsdb",
          "Replicating partition $0/$1/$2 to $3",
          snap_->state.tsdb_namespace(),
          snap_->state.table_key(),
          snap_->key.toString(),
          r.addr);

      try {
        replicateTo(r, replica_offset);
        setReplicatedOffsetFor(&repl_state, r.unique_id, head_offset);
        dirty = true;
      } catch (const std::exception& e) {
        success = false;

        stx::logError(
          "tsdb",
          e,
          "Error while replicating partition $0/$1/$2 to $3",
          snap_->state.tsdb_namespace(),
          snap_->state.table_key(),
          snap_->key.toString(),
          r.addr);
      }
    }
  }

  if (dirty) {
    commitReplicationState(repl_state);
  }

  return success;
}

ReplicationState PartitionReplication::fetchReplicationState() const {
  auto filepath = FileUtil::joinPaths(snap_->base_path, kStateFileName);

  if (FileUtil::exists(filepath)) {
    return msg::decode<ReplicationState>(FileUtil::read(filepath));
  } else {
    ReplicationState state;
    auto uuid = snap_->uuid();
    state.set_uuid(uuid.data(), uuid.size());
    return state;
  }
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

} // namespace tdsb

