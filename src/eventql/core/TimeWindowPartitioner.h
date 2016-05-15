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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/option.h>
#include <eventql/util/UnixTime.h>
#include <eventql/util/duration.h>
#include <eventql/util/SHA1.h>
#include <eventql/core/TablePartitioner.h>
#include <eventql/core/TableConfig.pb.h>

#include "eventql/eventql.h"

namespace eventql {

struct TimeseriesPartition {
  UnixTime time_begin;
  UnixTime time_limit;
  SHA1Hash partition_key;
};

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

  static Vector<SHA1Hash> listPartitions(
      const String& table_name,
      UnixTime from,
      UnixTime until,
      Duration window_size);

  static Vector<TimeseriesPartition> partitionsFor(
      const String& table_name,
      UnixTime from,
      UnixTime until,
      Duration window_size);

  Vector<SHA1Hash> listPartitions(
      UnixTime from,
      UnixTime until);

  Vector<TimeseriesPartition> partitionsFor(
      UnixTime from,
      UnixTime until);

  SHA1Hash partitionKeyFor(const String& partition_key) const override;

  Vector<SHA1Hash> listPartitions(
      const Vector<csql::ScanConstraint>& constraints) const override;

protected:
  String table_name_;
  TimeWindowPartitionerConfig config_;
};

}
