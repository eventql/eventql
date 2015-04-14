/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_EVENTDB_TABLEREADER_H
#define _FNORD_EVENTDB_TABLEREADER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-msg/MessageObject.h>
#include <fnord-eventdb/TableArena.h>
#include <fnord-eventdb/TableSnapshot.h>
#include "fnord-sstable/sstablereader.h"
#include "fnord-cstable/CSTableReader.h"

namespace fnord {
namespace eventdb {

class TableReader : public RefCounted {
public:

  static RefPtr<TableReader> open(
      const String& table_name,
      const String& replica_id,
      const String& db_path,
      const msg::MessageSchema& schema);

  const String& name() const;

  RefPtr<TableSnapshot> getSnapshot();

protected:

  TableReader(
      const String& table_name,
      const String& replica_id,
      const String& db_path,
      const msg::MessageSchema& schema,
      uint64_t head_generation);

  String name_;
  String replica_id_;
  String db_path_;
  msg::MessageSchema schema_;
  std::mutex mutex_;
  uint64_t head_gen_;
};

} // namespace eventdb
} // namespace fnord

#endif
