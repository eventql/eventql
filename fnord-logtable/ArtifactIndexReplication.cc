/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-logtable/ArtifactIndexReplication.h>

namespace fnord {
namespace logtable {

ArtifactIndexReplication::ArtifactIndexReplication(
    RefPtr<ArtifactIndex> index,
    RefPtr<ArtifactIndexMergeStrategy> merge_strategy) :
    index_(index),
    merge_strategy_(merge_strategy) {}

void ArtifactIndexReplication::replicateFrom(
    const ArtifactIndexSnapshot& other) {
  index_->withIndex(false, [this, &other] (ArtifactIndexSnapshot* local) {
    merge_strategy_->merge(local, &other);
  });
}

void ArtifactIndexReplication::replicateFrom(
    const URI& uri,
    http::HTTPConnectionPool* http) {
  http::HTTPRequest req(http::HTTPMessage::M_GET, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  auto res = http->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 200) {
    RAISEF(kRuntimeError, "received non-200 response: $0", r.body().toString());
  }

  ArtifactIndexSnapshot remote;
  remote.decode(r.body());
  replicateFrom(remote);
}

} // namespace logtable
} // namespace fnord

