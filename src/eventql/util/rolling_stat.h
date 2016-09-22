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
#pragma once
#include "eventql/util/stdtypes.h"
#include "eventql/util/time_constants.h"

class RollingStat {
public:

  struct Bucket {
    uint64_t time;
    uint64_t value;
    uint64_t count;
  };

  static const uint64_t kDefaultNumBuckets = 1000;
  static const uint64_t kDefaultBucketIntervalUs = kMicrosPerSecond / 100;

  RollingStat(
      uint64_t buckets_capacity = kDefaultNumBuckets,
      uint64_t bucket_interval_us = kDefaultBucketIntervalUs);

  Bucket* getCurrent();

  void computeAggregate(uint64_t* value, uint64_t* count) const;
  void computeAggregate(double* quotient) const;

  uint64_t getBucketInterval() const;

protected:

  uint64_t buckets_capacity_;
  std::vector<Bucket> buckets_;
  uint64_t bucket_interval_us_;
  uint64_t buckets_idx_;
  uint64_t buckets_size_;
};

