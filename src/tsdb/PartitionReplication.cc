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

using namespace stx;

namespace tsdb {

bool PartitionReplication::needsReplication(RefPtr<Partition> partition) {
  return false;
}

void PartitionReplication::replicate(RefPtr<Partition> partition) {
  auto snap = partition->getSnapshot();
  auto table = partition->getTable();

  logDebug(
      "tsdb",
      "Replicating partition $0/$1/$2",
      snap->state.tsdb_namespace(),
      table->name(),
      snap->key.toString());

  //auto replicas = node_->replication_scheme->replicasFor(snap_.key());
/*

  auto cur_offset = records_.lastOffset();

  bool dirty = false;
  bool needs_replication = false;
  bool has_error = false;

  for (const auto& r : replicas) {
    auto& off = repl_offsets_[r.unique_id];

    if (off < cur_offset) {
      try {
        auto rep_offset = replicateTo(r.addr, off);
        dirty = true;
        off = rep_offset;
        if (off < cur_offset) {
          needs_replication = true;
        }
      } catch (const std::exception& e) {
        has_error = true;

        stx::logError(
          "tsdb.replication",
          e,
          "Error while replicating stream '$0' to '$1'",
          stream_key_,
          r.addr);
      }
    }
  }

  for (auto cur = repl_offsets_.begin(); cur != repl_offsets_.end(); ) {
    bool found;
    for (const auto& r : replicas) {
      if (r.unique_id == cur->first) {
        found = true;
        break;
      }
    }

    if (found) {
      ++cur;
    } else {
      cur = repl_offsets_.erase(cur);
      dirty = true;
    }
  }

  if (dirty) {
    std::unique_lock<std::mutex> lk(mutex_);
    commitState();
  }

  if (needs_replication) {
    node_->replicationq.insert(this, WallClock::unixMicros());
  } else if (has_error) {
    node_->replicationq.insert(
        this,
        WallClock::unixMicros() + 30 * kMicrosPerSecond);
  }
}

uint64_t Partition::replicateTo(const String& addr, uint64_t offset) {
  size_t batch_size = 1024;
  util::BinaryMessageWriter batch;

  auto start_offset = records_.firstOffset();
  if (start_offset > offset) {
    offset = start_offset;
  }

  size_t n = 0;
  records_.fetchRecords(offset, batch_size, [this, &batch, &n] (
      const SHA1Hash& record_id,
      const void* record_data,
      size_t record_size) {
    ++n;

    batch.append(record_id.data(), record_id.size());
    batch.appendVarUInt(record_size);
    batch.append(record_data, record_size);
  });

  stx::logDebug(
      "tsdb.replication",
      "Replicating to $0; stream='$1' partition='$2' offset=$3",
      addr,
      stream_key_,
      key_.toString(),
      offset);

  URI uri(StringUtil::format(
      "http://$0/tsdb/replicate?stream=$1&chunk=$2",
      addr,
      URI::urlEncode(stream_key_),
      URI::urlEncode(key_.toString())));

  http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  req.addHeader("Content-Type", "application/fnord-msg");
  req.addBody(batch.data(), batch.size());

  auto res = node_->http->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 201) {
    RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
  }

  return offset + n;
*/
}


} // namespace tdsb

