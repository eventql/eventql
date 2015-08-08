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
#include <tsdb/TimeWindowPartitioner.h>
#include <tsdb/FixedShardPartitioner.h>

using namespace stx;

namespace tsdb {

Table::Table(
    const TableDefinition& config) :
    config_(config) {
  loadConfig();
}

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

size_t Table::sstableSize() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.config().sstable_size();
}

size_t Table::numShards() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.config().num_shards();
}

RefPtr<msg::MessageSchema> Table::schema() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return schema_;
}

TableDefinition Table::config() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_;
}

TableStorage Table::storage() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.config().storage();
}

TablePartitioner Table::partitionerType() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.config().partitioner();
}

RefPtr<Partitioner> Table::partitioner() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return partitioner_;
}

void Table::updateConfig(TableDefinition new_config) {
  std::unique_lock<std::mutex> lk(mutex_);
  config_ = new_config;
  loadConfig();
}

void Table::loadConfig() {
  schema_ = msg::MessageSchema::decode(config_.config().schema());

  switch (config_.config().partitioner()) {

    case TBL_PARTITION_TIMEWINDOW:
      partitioner_ = RefPtr<Partitioner>(
          new TimeWindowPartitioner(config_.table_name()));
      break;

    case TBL_PARTITION_FIXED:
      partitioner_ = RefPtr<Partitioner>(
          new FixedShardPartitioner(config_.table_name()));
      break;

  }
}

}
