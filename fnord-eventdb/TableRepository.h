/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_EVENTDB_TABLEREPOSITORY_H
#define _FNORD_EVENTDB_TABLEREPOSITORY_H
#include <fnord-base/stdtypes.h>
#include <fnord-eventdb/TableReader.h>
#include <fnord-eventdb/TableWriter.h>
#include <fnord-eventdb/ArtifactIndex.h>
#include <fnord-msg/MessageSchema.h>

namespace fnord {
namespace eventdb {

class TableRepository {
public:

  TableRepository(
      ArtifactIndex* artifacts,
      const String& db_path,
      const String& replica_id,
      bool readonly);

  void addTable(const String& table_name, const msg::MessageSchema& schema);

  RefPtr<TableSnapshot> getSnapshot(const String& table_name) const;

  RefPtr<TableWriter> findTableWriter(const String& table_name) const;
  RefPtr<TableReader> findTableReader(const String& table_name) const;
  Set<String> tables() const;

  const String& replicaID() const;

protected:
  ArtifactIndex* artifacts_;
  String db_path_;
  String replica_id_;
  bool readonly_;
  HashMap<String, RefPtr<TableWriter>> table_writers_;
  HashMap<String, RefPtr<TableReader>> table_readers_;
  mutable std::mutex mutex_;
};

} // namespace eventdb
} // namespace fnord

#endif
