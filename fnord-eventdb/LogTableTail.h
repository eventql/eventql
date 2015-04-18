/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_EVENTDB_LOGTABLETAIL_H
#define _FNORD_EVENTDB_LOGTABLETAIL_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-msg/MessageObject.h>
#include <fnord-eventdb/TableReader.h>

namespace fnord {
namespace eventdb {

struct LogTableTailOffset {
  String replica_id;
  uint64_t consumed_offset;
};

typedef Vector<LogTableTailOffset> LogTableTailCursor;

class LogTableTail : public RefCounted {
public:

  LogTableTail(RefPtr<TableReader> reader);
  LogTableTail(RefPtr<TableReader> reader, LogTableTailCursor cursor);

  bool fetchNext(
      Function<bool (const msg::MessageObject& record)> fn,
      size_t limit = -1);

  LogTableTailCursor getCursor() const;

protected:

  void readRowsFromTable(
      const TableChunkRef& table,
      size_t offset,
      size_t limit,
      Function<bool (const msg::MessageObject& record)> fn);


  RefPtr<TableReader> reader_;
  HashMap<String, uint64_t> offsets_;
};

} // namespace eventdb
} // namespace fnord

#endif
