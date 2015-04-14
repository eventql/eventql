/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_EVENTDB_TABLESNAPSHOT_H
#define _FNORD_EVENTDB_TABLESNAPSHOT_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-base/random.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-msg/MessageObject.h>
#include <fnord-eventdb/TableArena.h>

namespace fnord {
namespace eventdb {

struct TableChunkRef {
  String replica_id;
  String chunk_id;
  uint64_t start_sequence;
  uint64_t num_records;
  uint64_t sstable_checksum;
  uint64_t cstable_checksum;
  uint64_t index_checksum;
};

struct TableGeneration : public RefCounted {
  String table_name;
  uint64_t generation;
  Vector<TableChunkRef> chunks;

  TableGeneration();
  RefPtr<TableGeneration> clone() const;
  void encode(Buffer* buf);
  void decode(const Buffer& buf);
};

struct TableSnapshot : public RefCounted {
  TableSnapshot(
      RefPtr<TableGeneration> _head,
      List<RefPtr<TableArena>> _arenas);

  RefPtr<TableGeneration> head;
  List<RefPtr<TableArena>> arenas;
};

} // namespace eventdb
} // namespace fnord

#endif
