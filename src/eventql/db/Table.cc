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
#include <eventql/db/Table.h>
#include <eventql/db/TimeWindowPartitioner.h>
#include <eventql/db/FixedShardPartitioner.h>

#include "eventql/eventql.h"

namespace eventql {

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
  return config_.customer();
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

Duration Table::commitInterval() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.config().commit_interval();
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

const String& Table::getPartitionKey() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.config().partition_key();
}

TablePartitionerType Table::partitionerType() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.config().partitioner();
}

RefPtr<TablePartitioner> Table::partitioner() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return partitioner_;
}

Vector<String> Table::getPrimaryKey() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return Vector<String>(
      config_.config().primary_key().begin(),
      config_.config().primary_key().end());
}

MetadataTransaction Table::getLastMetadataTransaction() const {
  std::unique_lock<std::mutex> lk(mutex_);

  return MetadataTransaction(
      SHA1Hash(
          config_.metadata_txnid().data(),
          config_.metadata_txnid().size()),
      config_.metadata_txnseq());
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
                config_.config().partition_key(),
                config_.config().time_window_partitioner_config()));
      } else {
        partitioner_ = RefPtr<TablePartitioner>(
            new TimeWindowPartitioner(
                config_.table_name(),
                config_.config().partition_key()));
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
