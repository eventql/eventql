/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/exception.h>
#include <fnord-base/wallclock.h>
#include <fnord-msg/msg.h>
#include <fnord-metricdb/backends/tsdb/metric.h>
#include <fnord-metricdb/Sample.pb.h>

namespace fnord {
namespace metric_service {
namespace tsdb_backend {

using fnord::DateTime;
using fnord::WallClock;

Metric::Metric(
    const String& key,
    const String& tsdb_prefix,
    tsdb::TSDBClient* tsdb) :
    IMetric(key),
    tsdb_prefix_(tsdb_prefix),
    tsdb_(tsdb),
    total_bytes_(0),
    last_insert_time_(0) {}

void Metric::insertSampleImpl(
    double value,
    const std::vector<std::pair<std::string, std::string>>& labels) {
  auto stream_key = tsdb_prefix_ + key();

  metricd::SampleData smpl;
  smpl.set_value(value);

  for (const auto& l : labels){
    auto label = smpl.add_labels();
    label->set_key(l.first);
    label->set_value(l.second);
  }

  auto sample_time = WallClock::unixMicros();

  tsdb_->insertRecord(
      stream_key,
      sample_time,
      tsdb_->mkMessageID(),
      *msg::encode(smpl));
}

void Metric::scanSamples(
    const DateTime& time_begin,
    const DateTime& time_end,
    std::function<bool (Sample* sample)> callback) {
  RAISE(kNotYetImplementedError);
}

Sample Metric::getSample() {
  RAISE(kNotYetImplementedError);
}

size_t Metric::totalBytes() const {
  return total_bytes_;
}

DateTime Metric::lastInsertTime() const {
  return DateTime(last_insert_time_);
}

std::set<std::string> Metric::labels() const {
  std::lock_guard<std::mutex> lock_holder(labels_mutex_);
  return labels_;
}

bool Metric::hasLabel(const std::string& label) const {
  std::lock_guard<std::mutex> lock_holder(labels_mutex_);
  return labels_.find(label) != labels_.end();
}

}
}
}

