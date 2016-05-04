/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/util/binarymessagereader.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/logging.h>
#include <eventql/core/TimeWindowPartitioner.h>

using namespace stx;

namespace zbase {

TimeWindowPartitioner::TimeWindowPartitioner(
    const String& table_name) :
    table_name_(table_name) {
  config_.set_partition_size(4 * kMicrosPerHour);
}

TimeWindowPartitioner::TimeWindowPartitioner(
    const String& table_name,
    const TimeWindowPartitionerConfig& config) :
    table_name_(table_name),
    config_(config) {}

SHA1Hash TimeWindowPartitioner::partitionKeyFor(
    const String& table_name,
    UnixTime time,
    Duration window_size) {
  util::BinaryMessageWriter buf(table_name.size() + 32);

  auto cs = window_size.microseconds();
  if (cs == 0) {
    RAISE(kIllegalArgumentError, "time_window must be > 0");
  }

  auto ts = (time.unixMicros() / cs) * cs / kMicrosPerSecond;

  buf.append(table_name.data(), table_name.size());
  buf.appendUInt8(27);
  buf.appendVarUInt(ts);

  return SHA1::compute(buf.data(), buf.size());
}

Vector<SHA1Hash> TimeWindowPartitioner::listPartitions(
    const String& table_name,
    UnixTime from,
    UnixTime until,
    Duration window_size) {
  auto cs = window_size.microseconds();
  auto first_chunk = (from.unixMicros() / cs) * cs;
  auto last_chunk = ((until.unixMicros() + (cs - 1)) / cs) * cs;

  Vector<SHA1Hash> res;
  for (auto t = first_chunk; t <= last_chunk; t += cs) {
    res.emplace_back(partitionKeyFor(table_name, t, window_size));
  }

  return res;
}

Vector<TimeseriesPartition> TimeWindowPartitioner::partitionsFor(
    const String& table_name,
    UnixTime from,
    UnixTime until,
    Duration window_size) {
  auto cs = window_size.microseconds();
  auto first_chunk = (from.unixMicros() / cs) * cs;
  auto last_chunk = (until.unixMicros() / cs) * cs;

  Vector<TimeseriesPartition> res;
  for (auto t = first_chunk; t <= last_chunk; t += cs) {
    res.emplace_back(TimeseriesPartition {
      .time_begin = t,
      .time_limit = t + window_size.microseconds(),
      .partition_key = partitionKeyFor(table_name, t, window_size)
    });
  }

  return res;
}

Vector<SHA1Hash> TimeWindowPartitioner::listPartitions(
    UnixTime from,
    UnixTime until) {
  return listPartitions(
      table_name_,
      from,
      until,
      config_.partition_size());
}

Vector<TimeseriesPartition> TimeWindowPartitioner::partitionsFor(
    UnixTime from,
    UnixTime until) {
  return partitionsFor(
      table_name_,
      from,
      until,
      config_.partition_size());
}

SHA1Hash TimeWindowPartitioner::partitionKeyFor(
    const String& partition_key) const {
  return partitionKeyFor(
      table_name_,
      UnixTime(std::stoull(partition_key)),
      config_.partition_size());
}

Vector<SHA1Hash> TimeWindowPartitioner::listPartitions(
    const Vector<csql::ScanConstraint>& constraints) const {
  uint64_t lower_limit;
  bool has_lower_limit = false;
  uint64_t upper_limit;
  bool has_upper_limit = false;

  for (const auto& c : constraints) {
    if (c.column_name != "time") { // FIXME
      continue;
    }

    uint64_t val;
    try {
      val = c.value.getInteger();
    } catch (const StandardException& e) {
      continue;
    }

    switch (c.type) {
      case csql::ScanConstraintType::EQUAL_TO:
        lower_limit = val;
        upper_limit = val;
        has_lower_limit = true;
        has_upper_limit = true;
        break;
      case csql::ScanConstraintType::NOT_EQUAL_TO:
        break;
      case csql::ScanConstraintType::LESS_THAN:
        upper_limit = val - 1;
        has_upper_limit = true;
        break;
      case csql::ScanConstraintType::LESS_THAN_OR_EQUAL_TO:
        upper_limit = val;
        has_upper_limit = true;
        break;
      case csql::ScanConstraintType::GREATER_THAN:
        lower_limit = val + 1;
        has_lower_limit = true;
        break;
      case csql::ScanConstraintType::GREATER_THAN_OR_EQUAL_TO:
        lower_limit = val;
        has_lower_limit = true;
        break;
    }
  }

  if (!has_lower_limit || !has_upper_limit) {
    RAISE(
        kRuntimeError,
        "All queries on timeseries tables must have an upper and lower bound "
        "on the scanned time window. Try appending this to your query: "
        "'WHERE time > time_at(\"-30min\") AND time < time_at(\"now\")'");
  }

  return listPartitions(
        table_name_,
        lower_limit,
        upper_limit,
        config_.partition_size());
}

}
