/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_FEATURECACHE_H
#define _CM_FEATURECACHE_H
#include "fnord-base/stdtypes.h"
#include "fnord-rpc/RPC.h"
#include "fnord-rpc/RPCClient.h"
#include "fnord-base/thread/taskscheduler.h"
#include "fnord-mdb/MDB.h"
#include "fnord-base/stats/stats.h"
#include "FeaturePack.h"
#include "FeatureSchema.h"
#include "FeatureID.h"
#include "ItemRef.h"

using namespace fnord;

namespace cm {

class FeatureCache {
public:

  Option<String> get(const String& key);
  void store(const String& key, const String& value);

protected:
  HashMap<String, String> data_;
};
} // namespace cm

#endif
