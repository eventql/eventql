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
#ifndef _FNORD_LOGTABLE_LOGTABLETAIL_H
#define _FNORD_LOGTABLE_LOGTABLETAIL_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/util/binarymessagereader.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/util/protobuf/MessageObject.h>
#include <fnord-logtable/TableReader.h>

namespace util {
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
} // namespace util

#endif
