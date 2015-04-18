/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/logging.h>
#include <fnord-eventdb/LogTableTail.h>

namespace fnord {
namespace eventdb {

LogTableTail::LogTableTail(
    RefPtr<TableReader> reader) :
    reader_(reader) {
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
    RefPtr<TableReader> reader,
    LogTableTailCursor cursor) :
    reader_(reader) {
  for (const auto& o : cursor) {
    offsets_.emplace(o.replica_id, o.consumed_offset);
  }
}

bool LogTableTail::fetchNext(
    Function<bool (const msg::MessageObject& record)> fn,
    size_t limit /* = -1 */) {
  auto snap = reader_->getSnapshot();

  for (const auto& c : snap->head->chunks) {
    auto cbegin = c.start_sequence;
    auto cend = cbegin + c.num_records;
    auto cur = offsets_[c.replica_id];

    if (cbegin <= cur && cend > cur) {
      auto roff = cur - cbegin;
      auto rlen = c.num_records - roff;

      if (limit != size_t(-1) && rlen > limit) {
        rlen = limit;
      }

      readRowsFromTable(c, roff, rlen, fn);

      offsets_[c.replica_id] += rlen;
      return true;
    }
  }

  return false;
}

LogTableTailCursor LogTableTail::getCursor() const {
  LogTableTailCursor cursor;

  for (const auto& o : offsets_) {
    cursor.emplace_back(LogTableTailOffset {
      .replica_id = o.first,
      .consumed_offset = o.second
    });
  }

  return cursor;
}

void LogTableTail::readRowsFromTable(
    const TableChunkRef& table,
    size_t offset,
    size_t limit,
    Function<bool (const msg::MessageObject& record)> fn) {
  auto table_filename = StringUtil::format(
      "$0.$1.sst",
      table.replica_id,
      table.chunk_id);

#ifndef FNORD_NOTRACE
  fnord::logTrace(
      "fnord.evdb",
      "Reading rows local=$0..$1 global=$2..$3 from table '$4'",
      offset,
      offset + limit,
      table.start_sequence + offset,
      table.start_sequence + offset + limit,
      table_filename);
#endif

}


} // namespace eventdb
} // namespace fnord

