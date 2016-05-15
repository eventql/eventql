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
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/core/FixedShardPartitioner.h>

using namespace util;

namespace eventql {

FixedShardPartitioner::FixedShardPartitioner(
    const String& table_name,
    size_t num_shards) :
    table_name_(table_name),
    num_shards_(num_shards) {}

SHA1Hash FixedShardPartitioner::partitionKeyFor(
    const String& table_name,
    size_t shard) {
  util::BinaryMessageWriter buf(table_name.size() + 32);

  buf.append(table_name.data(), table_name.size());
  buf.appendUInt8(27);
  buf.appendVarUInt(shard);

  return SHA1::compute(buf.data(), buf.size());
}

Vector<SHA1Hash> FixedShardPartitioner::listPartitions(
    const String& table_name,
    size_t nshards) {
  Vector<SHA1Hash> partitions;

  for (size_t shard = 0; shard < nshards; ++shard) {
    partitions.emplace_back(partitionKeyFor(table_name, shard));
  }

  return partitions;
}

SHA1Hash FixedShardPartitioner::partitionKeyFor(
    const String& partition_key) const {
  return partitionKeyFor(table_name_, std::stoull(partition_key));
}

Vector<SHA1Hash> FixedShardPartitioner::listPartitions(
      const Vector<csql::ScanConstraint>& constraints) const {
  return listPartitions(table_name_, num_shards_);
}

}
