/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/Table.h>

using namespace stx;

namespace tsdb {

Table::Table(
    const TableDefinition& config,
    RefPtr<msg::MessageSchema> schema) :
    config_(config),
    schema_(schema) {}

String Table::name() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.table_name();
}

String Table::tsdbNamespace() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.tsdb_namespace();
}

Duration Table::partitionSize() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return 4 * kMicrosPerHour;
}

Duration Table::compactionInterval() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.config().compaction_interval();
}

size_t Table::sstableSize() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.config().sstable_size();
}

RefPtr<msg::MessageSchema> Table::schema() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return schema_;
}

TableDefinition Table::config() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_;
}

void Table::updateSchema(RefPtr<msg::MessageSchema> new_schema) {
  std::unique_lock<std::mutex> lk(mutex_);
  schema_ = new_schema;
}

void Table::updateConfig(TableDefinition new_config) {
  std::unique_lock<std::mutex> lk(mutex_);
  config_ = new_config;
}

}
