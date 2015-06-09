/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord-metricdb/metricservice.h"
#include "fnord-base/wallclock.h"
#include "fnord-msg/msg.h"

namespace fnord {
namespace metric_service {

MetricService::MetricService(
      const String& tsdb_prefix,
      tsdb::TSDBClient* tsdb) :
      tsdb_prefix_(tsdb_prefix),
      tsdb_(tsdb) {}

std::vector<IMetric*> MetricService::listMetrics() const {
  RAISE(kNotYetImplementedError);
}

void MetricService::insertSample(
    const std::string& metric_key,
    double value,
    const std::vector<std::pair<std::string, std::string>>& labels) {
  auto stream_key = tsdb_prefix_ + metric_key;
  auto sample_time = WallClock::unixMicros();

  metricd::MetricSample smpl;
  smpl.set_value(value);
  smpl.set_time(sample_time);

  for (const auto& l : labels){
    auto label = smpl.add_labels();
    label->set_key(l.first);
    label->set_value(l.second);
  }

  tsdb_->insertRecord(
      stream_key,
      sample_time,
      tsdb_->mkMessageID(),
      *msg::encode(smpl));
}

void MetricService::scanSamples(
    const std::string& metric_key,
    const fnord::DateTime& time_begin,
    const fnord::DateTime& time_end,
    std::function<void (const metricd::MetricSample& sample)> callback) {
  auto stream_key = tsdb_prefix_ + metric_key;

  auto partitions = tsdb_->listPartitions(stream_key, time_begin, time_end);

  for (const auto& partition_key : partitions) {
    tsdb_->fetchPartition(
        stream_key,
        partition_key,
        [callback] (const Buffer& buf) {
      callback(msg::decode<metricd::MetricSample>(buf));
    });
  }
}

Sample MetricService::getMostRecentSample(const std::string& metric_key) {
  RAISE(kNotYetImplementedError);
}

} // namespace metric_service
} // namsepace fnord

