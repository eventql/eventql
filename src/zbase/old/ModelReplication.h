/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_MODELREPLICATION_H
#define _CM_MODELREPLICATION_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <stx/autoref.h>
#include <stx/uri.h>
#include "fnord-afx/ArtifactReplication.h"
#include "fnord-afx/ArtifactIndexReplication.h"

using namespace stx;

namespace zbase {

class ModelReplication {
public:

  ModelReplication();

  void addJob(const String& name, Function<void()> fn);

  void addArtifactIndexReplication(
      const String& index_name,
      const String& artifact_dir,
      Vector<URI> sources,
      logtable::ArtifactReplication* replication,
      http::HTTPConnectionPool* http);

  void start();
  void stop();

protected:
  void run();

  Duration interval_;
  bool running_;
  std::thread thread_;
  List<Pair<String, Function<void()>>> jobs_;
};

} // namespace zbase

#endif
