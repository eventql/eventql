/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_LOGTABLETAIL_H
#define _FNORD_LOGTABLE_LOGTABLETAIL_H
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <stx/util/binarymessagereader.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/protobuf/MessageSchema.h>
#include <stx/protobuf/MessageObject.h>
#include <fnord-logtable/TableReader.h>

namespace stx {
namespace logtable {

struct LogTableTailOffset {
  String replica_id;
  uint64_t consumed_offset;
};

struct LogTableTailCursor {
  Vector<LogTableTailOffset> offsets;

  void encode(util::BinaryMessageWriter* writer) const;
  void decode(util::BinaryMessageReader* reader);

  String debugPrint() const;
};

class LogTableTail : public RefCounted {
public:

  LogTableTail(RefPtr<AbstractTableReader> reader);
  LogTableTail(RefPtr<AbstractTableReader> reader, LogTableTailCursor cursor);

  bool fetchNext(
      Function<bool (const msg::MessageObject& record)> fn,
      size_t limit = -1);

  LogTableTailCursor getCursor() const;

protected:
  RefPtr<AbstractTableReader> reader_;
  HashMap<String, uint64_t> offsets_;
  std::atomic<size_t> rr_;
  RefPtr<TableGeneration> cur_snap_;
};

} // namespace logtable
} // namespace stx

#endif
