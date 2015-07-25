/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-logtable/TableRepository.h>

namespace stx {
namespace logtable {

TableRepository::TableRepository(
    const String& db_path,
    const String& replica_id,
    bool readonly,
    TaskScheduler* scheduler) :
    db_path_(db_path),
    replica_id_(replica_id),
    readonly_(readonly),
    scheduler_(scheduler) {}

void TableRepository::addTable(
    const String& table_name,
    const msg::MessageSchema& schema) {
  std::unique_lock<std::mutex> lk(mutex_);

  table_readers_.emplace(
      table_name,
      TableReader::open(
          table_name,
          replica_id_,
          db_path_,
          schema));

  if (!readonly_) {
    table_writers_.emplace(
        table_name,
        TableWriter::open(
            table_name,
            replica_id_,
            db_path_,
            schema,
            scheduler_));
  }
}

RefPtr<TableWriter> TableRepository::findTableWriter(
    const String& table_name) const {
  if (readonly_) {
    RAISEF(kRuntimeError, "table is in read-only mode: '$0'", table_name);
  }

  std::unique_lock<std::mutex> lk(mutex_);
  auto table = table_writers_.find(table_name);
  if (table == table_writers_.end()) {
    RAISEF(kIndexError, "unknown table: '$0'", table_name);
  }

  return table->second;
}

RefPtr<TableReader> TableRepository::findTableReader(
    const String& table_name) const {
  std::unique_lock<std::mutex> lk(mutex_);
  auto table = table_readers_.find(table_name);
  if (table == table_readers_.end()) {
    RAISEF(kIndexError, "unknown table: '$0'", table_name);
  }

  return table->second;
}

RefPtr<TableSnapshot> TableRepository::getSnapshot(
    const String& table_name) const {
  std::unique_lock<std::mutex> lk(mutex_);

  if (readonly_) {
    auto table = table_readers_.find(table_name);
    if (table == table_readers_.end()) {
      RAISEF(kIndexError, "unknown table: '$0'", table_name);
    }

    return table->second->getSnapshot();
  } else {
    auto table = table_writers_.find(table_name);
    if (table == table_writers_.end()) {
      RAISEF(kIndexError, "unknown table: '$0'", table_name);
    }

    return table->second->getSnapshot();
  }
}

Set<String> TableRepository::tables() const {
  Set<String> tables;

  std::unique_lock<std::mutex> lk(mutex_);
  for (auto& t : table_writers_) {
    tables.emplace(t.first);
  }

  return tables;
}

const String& TableRepository::replicaID() const {
  return replica_id_;
}

} // namespace logtable
} // namespace stx
