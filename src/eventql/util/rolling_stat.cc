/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <assert.h>
#include "eventql/util/rolling_stat.h"
#include "eventql/util/wallclock.h"

RollingStat::RollingStat(
    uint64_t buckets_capacity /* = kDefaultNumBuckets */,
    uint64_t bucket_interval_us /* = kDefaultBucketIntervalUs */) :
    buckets_capacity_(buckets_capacity),
    buckets_(buckets_capacity_),
    bucket_interval_us_(bucket_interval_us),
    buckets_idx_(0),
    buckets_size_(0) {}

void RollingStat::addValue(uint64_t val) {
  auto cur_time =
      (MonotonicClock::now() / bucket_interval_us_) * bucket_interval_us_;

  if (buckets_size_ == 0 || buckets_[buckets_idx_].time != cur_time) {
    buckets_idx_ = (buckets_idx_ + 1) % buckets_capacity_;
    if (buckets_size_ < buckets_capacity_) {
      ++buckets_size_;
    }

    buckets_[buckets_idx_].time = cur_time;
    buckets_[buckets_idx_].value = 0;
    buckets_[buckets_idx_].count = 0;
  }

  assert(buckets_idx_ < buckets_.size());
  buckets_[buckets_idx_].value += val;
  ++buckets_[buckets_idx_].count;
}

void RollingStat::computeAggregate(
    uint64_t* value,
    uint64_t* count,
    uint64_t* interval_us) const {
  auto time_now = MonotonicClock::now();
  auto time_range = bucket_interval_us_ * buckets_capacity_;
  auto time_begin = time_now;
  if (time_begin >= time_range) {
    time_begin -= time_range;
  } else {
    time_begin = 0;
  }

  *value = 0;
  *count = 0;
  auto idx =
      (buckets_idx_ + (buckets_capacity_ - buckets_size_) + 1) %
      buckets_capacity_;

  if (buckets_size_ == 0) {
    *interval_us = 0;
  } else {
    *interval_us = time_now - buckets_[idx].time;
  }

  for (uint64_t i = 0; i < buckets_size_; ++i) {
    if (buckets_[idx].time >= time_begin) {
      *value += buckets_[idx].value;
      *count += buckets_[idx].count;
    }

    idx = (idx + 1) % buckets_capacity_;
  }
}

