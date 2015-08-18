/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <zbase/core/Table.h>
#include <zbase/core/TimeWindowPartitioner.h>
#include <zbase/core/FixedShardPartitioner.h>

using namespace stx;

namespace zbase {

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

Duration Table::cstableBuildInterval() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.config().cstable_build_interval();
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

TablePartitionerType Table::partitionerType() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.config().partitioner();
}

RefPtr<TablePartitioner> Table::partitioner() const {
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
      if (config_.config().has_time_window_partitioner_config()) {
        partitioner_ = RefPtr<TablePartitioner>(
            new TimeWindowPartitioner(
                config_.table_name(),
                config_.config().time_window_partitioner_config()));
      } else {
        partitioner_ = RefPtr<TablePartitioner>(
            new TimeWindowPartitioner(config_.table_name()));
      }
      break;

    case TBL_PARTITION_FIXED:
      partitioner_ = RefPtr<TablePartitioner>(
          new FixedShardPartitioner(
              config_.table_name(),
              config_.config().num_shards()));
      break;

  }
}

}
