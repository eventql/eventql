/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_TABLESNAPSHOT_H
#define _FNORD_LOGTABLE_TABLESNAPSHOT_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/random.h>
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/util/protobuf/MessageObject.h>
#include <fnord-logtable/TableArena.h>

namespace stx {
namespace logtable {

struct TableChunkRef {
  String replica_id;
  String chunk_id;
  uint64_t start_sequence;
  uint64_t num_records;
  uint64_t sstable_checksum;
  uint64_t cstable_checksum;
  uint64_t summary_checksum;
  uint64_t sstable_size;
  uint64_t cstable_size;
  uint64_t summary_size;
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
      RefPtr<TableGeneration> _head = new TableGeneration(),
      List<RefPtr<TableArena>> _arenas = List<RefPtr<TableArena>>());

  RefPtr<TableGeneration> head;
  List<RefPtr<TableArena>> arenas;
};

} // namespace logtable
} // namespace stx

#endif
