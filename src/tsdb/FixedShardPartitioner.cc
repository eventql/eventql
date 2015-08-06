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
#include <tsdb/FixedShardPartitioner.h>

using namespace stx;

namespace tsdb {

SHA1Hash FixedShardPartitioner::partitionKeyFor(
    const String& stream_key,
    size_t shard) {
  util::BinaryMessageWriter buf(stream_key.size() + 32);

  buf.append(stream_key.data(), stream_key.size());
  buf.appendUInt8(27);
  buf.appendVarUInt(shard);

  return SHA1::compute(buf.data(), buf.size());
}

Vector<SHA1Hash> FixedShardPartitioner::partitionKeysFor(
    const String& stream_key,
    size_t nshards) {
  Vector<SHA1Hash> partitions;

  for (size_t shard = 0; shard < nshards; ++shard) {
    partitions.emplace_back(partitionKeyFor(stream_key, shard));
  }

  return partitions;
}

}
