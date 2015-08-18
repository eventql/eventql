/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/wallclock.h>
#include <zbase/AnalyticsTableScanPlanner.h>
#include <zbase/core/TimeWindowPartitioner.h>
#include <zbase/core/TSDBTableScanSpec.pb.h>

using namespace stx;

namespace zbase {

List<dproc::TaskDependency> AnalyticsTableScanPlanner::mapShards(
    const String& customer,
    const UnixTime& from,
    const UnixTime& until,
    const String& task_name,
    const Buffer& task_params,
    zbase::TSDBService* tsdb) {
  return mapShards(
      customer,
      "sessions",
      "cm.JoinedSession",
      from,
      until,
      task_name,
      task_params,
      tsdb);
}

List<dproc::TaskDependency> AnalyticsTableScanPlanner::mapShards(
    const String& customer,
    const String& table_name,
    const String& schema_name,
    const UnixTime& from,
    const UnixTime& until,
    const String& task_name,
    const Buffer& task_params,
    zbase::TSDBService* tsdb) {
  auto cs = 4 * kMicrosPerHour;
  auto first_chunk = (from.unixMicros() / cs) * cs;
  auto last_chunk = (until.unixMicros() / cs) * cs;
  auto recent_cutoff = WallClock::unixMicros() - cs * 2;

  List<dproc::TaskDependency> deps;
  for (auto t = first_chunk; t <= last_chunk; t += cs) {
    auto partition = zbase::TimeWindowPartitioner::partitionKeyFor(
        table_name,
        t,
        cs);

    auto pinfo = tsdb->partitionInfo(
        customer,
        table_name,
        partition);

    if (pinfo.isEmpty()) {
      continue;
    }

    zbase::TSDBTableScanSpec spec;
    spec.set_tsdb_namespace(customer);
    spec.set_table_name(table_name);
    spec.set_partition_key(partition.toString());
    spec.set_schema_name(schema_name);
    spec.set_version(pinfo.get().checksum());
    spec.set_scanlet_params(task_params.toString());
    spec.set_use_cstable_index(t < recent_cutoff);

    deps.emplace_back(dproc::TaskDependency {
      .task_name = task_name,
      .params = *msg::encode(spec)
    });
  }

  return deps;
}

} // namespace zbase

