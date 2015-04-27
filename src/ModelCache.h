/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_MODELCACHE_H
#define _CM_MODELCACHE_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <fnord-base/autoref.h>
#include <fnord-base/uri.h>
#include <fnord-logtable/ArtifactIndex.h>

using namespace fnord;

namespace cm {

class ModelCache {
public:

  ModelCache(const String& datadir);

  RefCounted* getModel(
      const String& index_name,
      const String& prefix,
      Function<RefCounted* (const String& filename)> load_model);

protected:
  struct ModelRef {
    RefCounted* ptr;
  };

  String getLatestModelFilename(
      const String& index_name,
      const String& prefix);

  RefPtr<logtable::ArtifactIndex> getArtifactIndex(const String& index_name);

  String datadir_;
  HashMap<String, ModelRef> models_;
  HashMap<String, RefPtr<logtable::ArtifactIndex>> artifacts_;
};

} // namespace cm

#endif
