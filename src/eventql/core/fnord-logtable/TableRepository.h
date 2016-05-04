/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_TABLEREPOSITORY_H
#define _FNORD_LOGTABLE_TABLEREPOSITORY_H
#include <eventql/util/stdtypes.h>
#include <fnord-logtable/TableReader.h>
#include <fnord-logtable/TableWriter.h>
#include <fnord-afx/ArtifactIndex.h>
#include <eventql/util/protobuf/MessageSchema.h>

namespace stx {
namespace logtable {

class TableRepository {
public:

  TableRepository(
      const String& db_path,
      const String& replica_id,
      bool readonly,
      TaskScheduler* scheduler);

  void addTable(const String& table_name, const msg::MessageSchema& schema);

  RefPtr<TableSnapshot> getSnapshot(const String& table_name) const;

  RefPtr<TableWriter> findTableWriter(const String& table_name) const;
  RefPtr<TableReader> findTableReader(const String& table_name) const;
  Set<String> tables() const;

  const String& replicaID() const;

protected:
  String db_path_;
  String replica_id_;
  bool readonly_;
  HashMap<String, RefPtr<TableWriter>> table_writers_;
  HashMap<String, RefPtr<TableReader>> table_readers_;
  mutable std::mutex mutex_;
  TaskScheduler* scheduler_;
};

} // namespace logtable
} // namespace stx

#endif
