/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "unistd.h"
#include <stx/wallclock.h>
#include <stx/logging.h>
#include "ModelReplication.h"

using namespace stx;

namespace zbase {

ModelReplication::ModelReplication() :
    interval_(kMicrosPerSecond),
    running_(true) {}

void ModelReplication::addJob(const String& name, Function<void()> fn) {
  jobs_.emplace_back(name, fn);
}

void ModelReplication::addArtifactIndexReplication(
    const String& index_name,
    const String& artifact_dir,
    Vector<URI> sources,
    logtable::ArtifactReplication* replication,
    http::HTTPConnectionPool* http) {
  RefPtr<logtable::ArtifactIndex> index(
      new logtable::ArtifactIndex(artifact_dir, index_name, false));

  index->runConsistencyCheck(false, true);

  RefPtr<logtable::ArtifactIndexReplication> repl(
      new logtable::ArtifactIndexReplication(
          index,
          new logtable::AppendOnlyMergeStrategy()));

  replication->replicateArtifactsFrom(
      index.get(),
      sources);

  addJob(index_name, [index, repl, sources, http, index_name] () {
    for (const auto& s : sources) {
      URI suri(StringUtil::format("$0/$1.afx", s.toString(), index_name));
      repl->replicateFrom(suri, http);
    }
  });
}

void ModelReplication::start() {
  running_ = true;
  thread_ = std::thread(std::bind(&ModelReplication::run, this));
}

void ModelReplication::stop() {
  if (!running_) {
    return;
  }

  running_ = false;
  thread_.join();
}

void ModelReplication::run() {
  while (running_) {
    auto begin = WallClock::unixMicros();

    for (const auto& job : jobs_) {
      try {
        job.second();
      } catch (const Exception& e) {
        stx::logError(
            "cm.chunkserver",
            e,
            "ModelReplication error for model '$0'",
            job.first);
      }
    }

    auto elapsed = WallClock::unixMicros() - begin;
    if (elapsed < interval_.microseconds()) {
      usleep(interval_.microseconds() - elapsed);
    }
  }
}


} // namespace zbase

