/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/fnv.h>
#include <stx/exception.h>
#include "zbase/logjoin/LogJoinShard.h"

namespace zbase {

bool LogJoinShard::testUID(const String& uid) {
  stx::FNV<uint64_t> fnv;
  auto h = fnv.hash(uid.c_str(), uid.length());
  auto m = h % LogJoinShard::modulo;
  return m >= begin && m < end;
}

LogJoinShardMap::LogJoinShardMap() {
  addShard("shard-1",  0,    512);
  addShard("shard-2",  512,  1024);
  addShard("shard-3",  1024, 1536);
  addShard("shard-4",  1536, 2048);
  addShard("shard-5",  2048, 2560);
  addShard("shard-6",  2560, 3072);
  addShard("shard-7",  3072, 3584);
  addShard("shard-8",  3584, 4096);
  addShard("shard-9",  4096, 4608);
  addShard("shard-10", 4608, 5120);
  addShard("shard-11", 5120, 5632);
  addShard("shard-12", 5632, 6144);
  addShard("shard-13", 6144, 6656);
  addShard("shard-14", 6656, 7168);
  addShard("shard-15", 7168, 7680);
  addShard("shard-16", 7680, 8192);
}

LogJoinShard LogJoinShardMap::getShard(const String& name) {
  auto iter = shards_.find(name);
  if (iter == shards_.end()) {
    RAISEF(kIndexError, "invalid shard: $0", name);
  }

  return iter->second;
}

void LogJoinShardMap::addShard(
    const String& name,
    uint64_t begin,
    uint64_t end) {
  LogJoinShard shard;
  shard.shard_name = name;
  shard.begin = begin;
  shard.end = end;
  shards_[name] = shard;
}

}
