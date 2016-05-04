/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/util/binarymessagewriter.h>
#include <eventql/core/FixedShardPartitioner.h>

using namespace stx;

namespace zbase {

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
