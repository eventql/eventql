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
#include <stx/autoref.h>
#include <stx/uri.h>
#include <fnord-afx/ArtifactIndex.h>

using namespace stx;

namespace zbase {

class ModelCache {
public:

  ModelCache(const String& datadir);

  RefCounted* getModel(
      const String& factory_name,
      const String& index_name,
      const String& prefix);

  void addModelFactory(
      const String& factory_name,
      Function<RefCounted* (const String& filename)> fn);

protected:

  RefCounted* loadModel(
      const String& factory_name,
      const String& index_name,
      const String& prefix,
      bool lock);

  String getLatestModelFilename(
      const String& index_name,
      const String& prefix);

  RefPtr<logtable::ArtifactIndex> getArtifactIndex(const String& index_name);

  std::mutex mutex_;
  String datadir_;
  HashMap<String, RefCounted*> models_;
  HashMap<String, RefPtr<logtable::ArtifactIndex>> artifacts_;
  HashMap<String, Function<RefCounted* (const String& filename)>> factories_;
};

} // namespace zbase

#endif
