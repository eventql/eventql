/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-eventdb/TableRepository.h>

namespace fnord {
namespace eventdb {

TableRepository::TableRepository(
    const String& db_path,
    const String& replica_id,
    bool readonly) :
    db_path_(db_path),
    replica_id_(replica_id),
    readonly_(readonly) {}

void TableRepository::addTable(
    const String& table_name,
    const msg::MessageSchema& schema) {
  std::unique_lock<std::mutex> lk(mutex_);

  if (!readonly_) {
    auto table_writer = TableWriter::open(
          table_name,
          replica_id_,
          db_path_,
          schema);

    table_writers_.emplace(table_name, table_writer);
  }
}

RefPtr<TableWriter> TableRepository::findTableWriter(const String& name) const {
  std::unique_lock<std::mutex> lk(mutex_);
  auto table = table_writers_.find(name);
  if (table == table_writers_.end()) {
    RAISEF(kIndexError, "unknown table: '$0'", name);
  }

  return table->second;
}

Set<String> TableRepository::tables() const {
  Set<String> tables;

  std::unique_lock<std::mutex> lk(mutex_);
  for (auto& t : table_writers_) {
    tables.emplace(t.first);
  }

  return tables;
}

} // namespace eventdb
} // namespace fnord

