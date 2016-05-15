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
#include <eventql/util/logging.h>
#include <fnord-logtable/LogTableTail.h>

namespace stx {
namespace logtable {

void LogTableTailCursor::encode(util::BinaryMessageWriter* writer) const {
  writer->appendVarUInt(offsets.size());
  for (const auto& o : offsets) {
    writer->appendVarUInt(o.replica_id.size());
    writer->append(o.replica_id.data(), o.replica_id.size());
    writer->appendVarUInt(o.consumed_offset);
  }
}

void LogTableTailCursor::decode(util::BinaryMessageReader* reader) {
  auto n = reader->readVarUInt();
  for (int i = 0; i < n; ++i) {
    auto rid_len = reader->readVarUInt();
    String replica_id((char *) reader->read(rid_len), rid_len);
    auto consumed_offset = reader->readVarUInt();

    offsets.emplace_back(LogTableTailOffset {
      .replica_id = replica_id,
      .consumed_offset = consumed_offset
    });
  }
}

LogTableTail::LogTableTail(
    RefPtr<AbstractTableReader> reader) :
    reader_(reader),
    cur_snap_(nullptr) {
  auto snap = reader_->getSnapshot();

  for (const auto& c : snap->head->chunks) {
    const auto& replica = c.replica_id;
    auto off = offsets_.find(replica);

    if (off == offsets_.end()) {
      offsets_[replica] = c.start_sequence;
    } else if (c.start_sequence < off->second) {
      off->second = c.start_sequence;
    }
  }
}

LogTableTail::LogTableTail(
    RefPtr<AbstractTableReader> reader,
    LogTableTailCursor cursor) :
    reader_(reader),
    cur_snap_(nullptr) {
  for (const auto& o : cursor.offsets) {
    offsets_.emplace(o.replica_id, o.consumed_offset);
  }
}

bool LogTableTail::fetchNext(
    Function<bool (const msg::MessageObject& record)> fn,
    size_t limit /* = -1 */) {
  if (cur_snap_.get() == nullptr) {
    cur_snap_ = reader_->getSnapshot()->head.get();
  }

  const auto& chunks = cur_snap_->chunks;

  size_t rr = ++rr_;
  for (size_t i = 0; i < chunks.size(); ++i) {
    const auto& c = chunks[(i + rr) % chunks.size()];
    auto cbegin = c.start_sequence;
    auto cend = cbegin + c.num_records;
    auto cur = offsets_[c.replica_id];

    if (cbegin <= cur && cend > cur) {
      auto roff = cur - cbegin;
      auto rlen = c.num_records - roff;

      if (limit != size_t(-1) && rlen > limit) {
        rlen = limit;
      }

      auto actual_rlen = reader_->fetchRecords(c.replica_id, cur, rlen, fn);
      offsets_[c.replica_id] += actual_rlen;
      return true;
    }
  }

  cur_snap_ = RefPtr<logtable::TableGeneration>(nullptr);
  return false;
}

LogTableTailCursor LogTableTail::getCursor() const {
  LogTableTailCursor cursor;

  for (const auto& o : offsets_) {
    cursor.offsets.emplace_back(LogTableTailOffset {
      .replica_id = o.first,
      .consumed_offset = o.second
    });
  }

  return cursor;
}

String LogTableTailCursor::debugPrint() const {
  Vector<String> str;

  for (const auto& o : offsets) {
    str.emplace_back(
        StringUtil::format("$0=$1", o.replica_id, o.consumed_offset));
  }

  return StringUtil::join(str, ", ");
}

} // namespace logtable
} // namespace stx

