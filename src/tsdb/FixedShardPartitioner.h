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
#include <stx/stdtypes.h>
#include <stx/option.h>
#include <stx/UnixTime.h>
#include <stx/duration.h>
#include <stx/SHA1.h>
#include <tsdb/TablePartitioner.h>

using namespace stx;

namespace tsdb {

class FixedShardPartitioner : public TablePartitioner {
public:

  FixedShardPartitioner(const String& table_name);

  static SHA1Hash partitionKeyFor(
      const String& table_name,
      size_t shard);

  static Vector<SHA1Hash> partitionKeysFor(
      const String& table_name,
      size_t nshards);

  SHA1Hash partitionKeyFor(const String& partition_key) const override;

protected:
  String table_name_;
};

}
