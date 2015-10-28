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
#include <zbase/core/TablePartitioner.h>
#include <zbase/core/TableConfig.pb.h>

using namespace stx;

namespace zbase {

class TimeWindowPartitioner : public TablePartitioner {
public:

  TimeWindowPartitioner(const String& table_name);

  TimeWindowPartitioner(
      const String& table_name,
      const TimeWindowPartitionerConfig& config);

  static SHA1Hash partitionKeyFor(
      const String& table_name,
      UnixTime time,
      Duration window_size);

  static Vector<SHA1Hash> partitionKeysFor(
      const String& table_name,
      UnixTime from,
      UnixTime until,
      Duration window_size);

  Vector<SHA1Hash> partitionKeysFor(
      UnixTime from,
      UnixTime until);

  SHA1Hash partitionKeyFor(const String& partition_key) const override;

  Vector<SHA1Hash> partitionKeysFor(
      const TSDBTableRef& table_ref) const override;

protected:
  String table_name_;
  TimeWindowPartitionerConfig config_;
};

}
