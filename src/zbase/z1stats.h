/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/stats/counter.h>
#include <stx/http/httpstats.h>

namespace zbase {

struct Z1Stats {
  stx::stats::Counter<uint64_t> num_partitions;
  stx::stats::Counter<uint64_t> num_partitions_loaded;
  stx::stats::Counter<uint64_t> replication_queue_length;
  stx::stats::Counter<uint64_t> compaction_queue_length;
  stx::http::HTTPClientStats http_client_stats;
};

Z1Stats* z1stats();

}
