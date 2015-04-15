/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_EVENTDB_ARTIFACTREPLICATION_H
#define _FNORD_EVENTDB_ARTIFACTREPLICATION_H
#include <thread>
#include <fnord-base/stdtypes.h>
#include <fnord-base/uri.h>
#include <fnord-eventdb/ArtifactIndex.h>
#include "fnord-http/httprequest.h"
#include "fnord-http/httpconnectionpool.h"

namespace fnord {
namespace eventdb {

class ArtifactReplication {
public:

  ArtifactReplication(
      ArtifactIndex* index,
      http::HTTPConnectionPool* http);

  void downloadPending();
  void start();
  void stop();

protected:

  void run();

  uint64_t interval_;
  std::atomic<bool> running_;
  std::thread thread_;

  ArtifactIndex* index_;
  http::HTTPConnectionPool* http_;
};

} // namespace eventdb
} // namespace fnord

#endif
