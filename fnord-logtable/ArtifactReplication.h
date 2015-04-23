/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_ARTIFACTREPLICATION_H
#define _FNORD_LOGTABLE_ARTIFACTREPLICATION_H
#include <thread>
#include <fnord-base/stdtypes.h>
#include <fnord-base/uri.h>
#include <fnord-logtable/ArtifactIndex.h>
#include "fnord-http/httprequest.h"
#include "fnord-http/httpconnectionpool.h"

namespace fnord {
namespace logtable {

class ArtifactReplication {
public:

  ArtifactReplication(
      http::HTTPConnectionPool* http,
      TaskScheduler* scheduler,
      size_t max_concurrent_reqs);

  void replicateArtifactsFrom(
      ArtifactIndex* index,
      const Vector<URI>& sources);

  void downloadPending();
  void start();
  void stop();

protected:

  void run();

  void enqueueArtifactDownload(
      const ArtifactRef& artifact,
      ArtifactIndex* index,
      const Vector<URI>& sources);

  void downloadArtifact(
      const ArtifactRef& artifact,
      ArtifactIndex* index,
      const Vector<URI>& sources);

  void downloadFile(
      const ArtifactFileRef& file,
      ArtifactIndex* index,
      const URI& uri);

  ArtifactIndex* index_;
  http::HTTPConnectionPool* http_;
  TaskScheduler* scheduler_;
  size_t max_concurrent_reqs_;

  uint64_t interval_;
  std::atomic<bool> running_;
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cv_;
  Set<String> cur_downloads_;

  Vector<Pair<ArtifactIndex*, Vector<URI>>> indexes_;
  std::atomic<uint64_t> rr_;

  std::mutex retry_mutex_;;
  HashMap<String, uint64_t> retry_map_;
};

} // namespace logtable
} // namespace fnord

#endif
