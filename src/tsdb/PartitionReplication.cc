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
#include <stx/logging.h>
#include <stx/io/fileutil.h>
#include <stx/protobuf/msg.h>

using namespace stx;

namespace tsdb {

const char PartitionReplication::kStateFileName[] = "_repl";

PartitionReplication::PartitionReplication(
    RefPtr<Partition> partition,
    RefPtr<ReplicationScheme> repl_scheme) :
    partition_(partition),
    snap_(partition_->getSnapshot()),
    repl_scheme_(repl_scheme) {}

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
        replicateTo(r);
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

//uint64_t Partition::replicateTo(const String& addr, uint64_t offset) {
//  size_t batch_size = 1024;
//  util::BinaryMessageWriter batch;
//
//  auto start_offset = records_.firstOffset();
//  if (start_offset > offset) {
//    offset = start_offset;
//  }
//
//  size_t n = 0;
//  records_.fetchRecords(offset, batch_size, [this, &batch, &n] (
//      const SHA1Hash& record_id,
//      const void* record_data,
//      size_t record_size) {
//    ++n;
//
//    batch.append(record_id.data(), record_id.size());
//    batch.appendVarUInt(record_size);
//    batch.append(record_data, record_size);
//  });
//
//  stx::logDebug(
//      "tsdb.replication",
//      "Replicating to $0; stream='$1' partition='$2' offset=$3",
//      addr,
//      stream_key_,
//      key_.toString(),
//      offset);
//
//  URI uri(StringUtil::format(
//      "http://$0/tsdb/replicate?stream=$1&chunk=$2",
//      addr,
//      URI::urlEncode(stream_key_),
//      URI::urlEncode(key_.toString())));
//
//  http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
//  req.addHeader("Host", uri.hostAndPort());
//  req.addHeader("Content-Type", "application/fnord-msg");
//  req.addBody(batch.data(), batch.size());
//
//  auto res = node_->http->executeRequest(req);
//  res.wait();
//
//  const auto& r = res.get();
//  if (r.statusCode() != 201) {
//    RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
//  }
//
//  return offset + n;
//*/
//}

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

