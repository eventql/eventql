/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/option.h>
#include <eventql/util/UnixTime.h>
#include <eventql/util/duration.h>
#include <eventql/util/SHA1.h>
#include <eventql/core/TablePartitioner.h>

using namespace stx;

namespace zbase {

class FixedShardPartitioner : public TablePartitioner {
public:

  FixedShardPartitioner(const String& table_name, size_t num_shards);

  static SHA1Hash partitionKeyFor(
      const String& table_name,
      size_t shard);

  static Vector<SHA1Hash> listPartitions(
      const String& table_name,
      size_t nshards);

  SHA1Hash partitionKeyFor(const String& partition_key) const override;

  Vector<SHA1Hash> listPartitions(
      const Vector<csql::ScanConstraint>& constraints) const override;

protected:
  String table_name_;
  size_t num_shards_;
};

}
