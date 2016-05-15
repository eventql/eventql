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
#include <fnord-logtable/TableRepository.h>

namespace util {
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
} // namespace util
