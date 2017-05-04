/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/db/table.h>
#include "eventql/eventql.h"

namespace eventql {

Table::Table(
    const TableDefinition& config) :
    config_(config) {
  schema_ = msg::MessageSchema::decode(config_.config().schema());
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

KeyspaceType Table::getKeyspaceType() const {
  switch (config_.config().partitioner()) {

    case TBL_PARTITION_TIMEWINDOW:
    case TBL_PARTITION_UINT64:
      return KEYSPACE_UINT64;
      break;

    case TBL_PARTITION_STRING:
      return KEYSPACE_STRING;
      break;

    case TBL_PARTITION_FIXED:
      RAISE(kIllegalArgumentError, "static partitions don't have a keyspace");
      break;

  }
}

const String& Table::getPartitionKey() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.config().partition_key();
}

TablePartitionerType Table::partitionerType() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.config().partitioner();
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

bool Table::hasUserDefinedPartitions() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return config_.config().enable_user_defined_partitions();
}

EVQL_CLEVEL_WRITE Table::getDefaultWriteConsistencyLevel() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return (EVQL_CLEVEL_WRITE) config_.config().default_write_consistency_level();
}

void Table::updateConfig(TableDefinition new_config) {
  std::unique_lock<std::mutex> lk(mutex_);
  config_ = new_config;
  schema_ = msg::MessageSchema::decode(config_.config().schema());
}

ReturnCode writeConsistencyLevelFromString(
    const std::string& str,
    EVQL_CLEVEL_WRITE* clevel) {
  std::string level_str = str;
  StringUtil::toUpper(&level_str);

  if (level_str == "STRICT") {
    *clevel = EVQL_CLEVEL_WRITE::EVQL_CLEVEL_WRITE_STRICT;
    return ReturnCode::success();
  } else if (level_str == "RELAXED") {
    *clevel = EVQL_CLEVEL_WRITE::EVQL_CLEVEL_WRITE_RELAXED;
    return ReturnCode::success();
  } else if (level_str == "BEST_EFFORT") {
    *clevel = EVQL_CLEVEL_WRITE::EVQL_CLEVEL_WRITE_BEST_EFFORT;
    return ReturnCode::success();
  } else {
    return ReturnCode::error(
        "eRuntimeError",
        StringUtil::format(
            "invalid write_consistency_level: $0",
            level_str));
  }
}

std::string writeConsistencyLevelToString(const EVQL_CLEVEL_WRITE& clevel) {
  switch (clevel) {
    case EVQL_CLEVEL_WRITE::EVQL_CLEVEL_WRITE_STRICT:
      return "STRICT";

    case EVQL_CLEVEL_WRITE::EVQL_CLEVEL_WRITE_RELAXED:
      return "RELAXED";

    case EVQL_CLEVEL_WRITE::EVQL_CLEVEL_WRITE_BEST_EFFORT:
      return "BEST EFFORT";
  }
}

}
