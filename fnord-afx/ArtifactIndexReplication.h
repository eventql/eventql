/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_ARTIFACTINDEXREPLICATION_H
#define _FNORD_LOGTABLE_ARTIFACTINDEXREPLICATION_H
#include <thread>
#include <fnord/stdtypes.h>
#include <fnord/uri.h>
#include <fnord-afx/ArtifactIndex.h>
#include <fnord-afx/ArtifactIndexMergeStrategy.h>
#include "fnord/http/httprequest.h"
#include "fnord/http/httpconnectionpool.h"

namespace fnord {
namespace logtable {

class ArtifactIndexReplication : public RefCounted {
public:
  ArtifactIndexReplication(
      RefPtr<ArtifactIndex> index,
      RefPtr<ArtifactIndexMergeStrategy> merge_strategy);

  void replicateFrom(const ArtifactIndexSnapshot& other);
  void replicateFrom(const URI& index_uri, http::HTTPConnectionPool* http);

protected:
  RefPtr<ArtifactIndex> index_;
  RefPtr<ArtifactIndexMergeStrategy> merge_strategy_;
};

} // namespace logtable
} // namespace fnord

#endif
