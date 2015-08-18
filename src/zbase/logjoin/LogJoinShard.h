/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_LOGJOINSHARD_H
#define _CM_LOGJOINSHARD_H
#include <stx/stdtypes.h>

using namespace stx;

namespace zbase {

struct LogJoinShard {
  static const uint64_t modulo = 8192;
  String shard_name;
  uint64_t begin;
  uint64_t end;

  bool testUID(const String& uid);
};

class LogJoinShardMap {
public:
  LogJoinShardMap();
  LogJoinShard getShard(const String& name);
  void addShard(const String& name, uint64_t begin, uint64_t end);
protected:
  HashMap<String, LogJoinShard> shards_;
};

} // namespace zbase

#endif
